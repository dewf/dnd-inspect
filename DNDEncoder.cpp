#include "DNDEncoder.h"

#include <TranslationKit.h>
#include <MediaKit.h>
#include <SupportKit.h>

#include <stdio.h>
#include <assert.h>

#include "Globals.h"

#include <map>
#include <string>

class DropFinalizer {
protected:
    std::map<std::string, int32> mimeToTypeCode;

    void addTranslations(BMessage *msg, BPositionIO *source, const char *nativeMIME, const char *nativeDesc)
    {
        static auto roster = BTranslatorRoster::Default();

        msg->AddString(K_FIELD_TYPES, nativeMIME);
        msg->AddString(K_FIELD_FILETYPES, nativeMIME);
        msg->AddString(K_FIELD_TYPE_DESCS, nativeDesc);

        translator_info *info;
        int32 numInfo;
        if (roster->GetTranslators(source, nullptr, &info, &numInfo) == B_OK) {
            for (int32 i=0; i< numInfo; i++) {
                const translation_format *formats;
                int32 count;
                if (roster->GetOutputFormats(info[i].translator, &formats, &count) == B_OK) {
                    for (int32 j = 0; j< count; j++) {
                        bool isNativeType = !strcmp(formats[j].MIME, nativeMIME);
                        if (isNativeType) {
                            // already been added
                            continue;
                        } else {
                            msg->AddString(K_FIELD_TYPES, formats[j].MIME);
                            msg->AddString(K_FIELD_FILETYPES, formats[j].MIME);
                            msg->AddString(K_FIELD_TYPE_DESCS, formats[j].name);

                            // add this translator to the map, so we know which code to use
                            // assert that it maps to the same code if we already have that type mapped ...
                            auto found = mimeToTypeCode.find(formats[j].MIME);
                            if (found != mimeToTypeCode.end()) {
                                assert(found->second == formats[j].type);
                            } else {
                                mimeToTypeCode[formats[j].MIME] = formats[j].type;
                            }
                        }
                    }
                }
            }
            // add final type indicating we support files
            msg->AddString(K_FIELD_TYPES, B_FILE_MIME_TYPE);

            // our responsibility to free
            delete[] info;
        }
    }
public:
    virtual void finalizeDrop(int32 action, const char *mimeType, BPositionIO *outStream) = 0;
};



class TranslationKitFinalizer : public DropFinalizer {
    BMallocIO storage;
public:
    TranslationKitFinalizer(BPositionIO *file, BMessage *msg, int32 baselineTypeCode, const char *mimeType, const char *desc) {
        storage.SetBlockSize(64 * 1024);
        auto roster = BTranslatorRoster::Default();
        if (roster->Translate(file, nullptr, nullptr, &storage, baselineTypeCode) == B_OK) {
            storage.Seek(0, SEEK_SET); // rewind
            addTranslations(msg, &storage, mimeType, desc);
        }
    }

    void finalizeDrop(int32 action, const char *mimeType, BPositionIO *outStream) override
    {
        switch(action) {
        case B_COPY_TARGET:
        case B_MOVE_TARGET:
        {
            // convert to whatever the user asked for
            auto typeCode = mimeToTypeCode[mimeType];
            printf("converting to typecode: %d\n", typeCode);

            auto roster = BTranslatorRoster::Default();
            if (roster->Translate(&storage, nullptr, nullptr, outStream, typeCode) == B_OK) {
                printf("successfully produced target type [%s]\n", mimeType);
            } else {
                printf("finalizeDropAsFile: failed to produce target type [%s]\n", mimeType);
            }
            break;
        }
        default:
            printf("not sure what to do for link/trash yet\n");
        }
    }
};

// ===================================

status_t DNDEncoder::readFileData(BFile *file, char **buffer, off_t *length)
{
    if (file->GetSize(length) == B_OK) {
        file->Seek(SEEK_SET, 0);

        *buffer = new char[*length];
        file->Read(*buffer, *length);

        file->Seek(0, SEEK_SET); // be kind, rewind
        return B_OK;
    } else {
        return B_ERROR;
    }
}

void DNDEncoder::addFileRef(const char *path)
{
    BEntry entry(path);
    entry_ref ref;
    if (entry.GetRef(&ref) == B_OK) {
        msg->AddRef("refs", &ref);
    }
}

void DNDEncoder::addNegotiatedPrologue()
{
    msg->AddPointer(K_FIELD_ORIGINATOR, this); // so the main window can easily forward replies to us
    msg->AddInt32(K_FIELD_ACTIONS, B_COPY_TARGET);
    msg->AddInt32(K_FIELD_ACTIONS, B_MOVE_TARGET);
    msg->AddInt32(K_FIELD_ACTIONS, B_TRASH_TARGET);
    msg->AddInt32(K_FIELD_ACTIONS, B_LINK_TARGET);
}

// ==== public methods ===============

DNDEncoder::DNDEncoder(BMessage *msg, const char *clipName)
    :msg(msg)
{
    msg->AddString(K_FIELD_CLIP_NAME, clipName);
}

DNDEncoder::~DNDEncoder()
{
    if (finalizer) {
        delete finalizer;
    }
}

void DNDEncoder::addTextFormat(const char *text)
{
    msg->SetData("text/plain", B_MIME_DATA, text, strlen(text));
}

void DNDEncoder::addTextFormat(BFile *file)
{
    // simple content
    char *buffer;
    off_t length;
    if (readFileData(file, &buffer, &length) == B_OK) {
        msg->SetData("text/plain", B_MIME_DATA, buffer, length);
        delete[] buffer;
    }

    // negotiated content
    addNegotiatedPrologue();

    finalizer = new TranslationKitFinalizer(file, msg, B_TRANSLATOR_TEXT, "text/plain", "Plain text file");
}

void DNDEncoder::addColor(rgb_color color)
{
    msg->SetData("RGBColor", B_RGB_COLOR_TYPE, &color, sizeof(color));
}

void DNDEncoder::addFileList(const char *fnames[], int count)
{
    for (int i=0; i< count; i++) {
        addFileRef(fnames[i]);
    }
}

void DNDEncoder::addBitmap(const char *path)
{
    // simple content
    addFileRef(path);

    addNegotiatedPrologue();

    BFile file(path, B_READ_ONLY);
    finalizer = new TranslationKitFinalizer(&file, msg, B_TRANSLATOR_BITMAP, "image/x-be-bitmap", "Be Bitmap Format");
}

void DNDEncoder::addFileContents(FileContent_t *files, int numFiles)
{
    addNegotiatedPrologue();

    for (int i=0; i < numFiles; i++) {
        auto fc = &files[i];
        msg->AddString(K_FIELD_TYPES, fc->mimeType);
        msg->AddString(K_FIELD_FILETYPES, fc->mimeType);
        msg->AddString(K_FIELD_TYPE_DESCS, fc->desc);
    }
    // add final type indicating we support files
    msg->AddString(K_FIELD_TYPES, B_FILE_MIME_TYPE);
}

void DNDEncoder::finalizeDrop(BMessage *request)
{
    BMessage reply(B_MIME_DATA);

    if (!finalizer) {
        printf("null finalizer in DNDEncoder::finalizeDrop! canceling\n");
        return;
    }

    // verify structure of msg etc
    auto action = request->what;
    if (action != B_COPY_TARGET && action != B_MOVE_TARGET && action != B_LINK_TARGET && action != B_TRASH_TARGET) {
        printf("DND negotiation action (message->what) was not one of: \n");
        printf("  B_COPY_TARGET\n  B_MOVE_TARGET\n  B_LINK_TARGET\n  B_TRASH_TARGET");
        return;
    }
    printf("negotiated action is %s\n", getActionString(action));

    // direct message or file?
    // plus we need to verify that we even offered to send it that way ...
    printf("=======\n");
    request->PrintToStream();
    printf("=======\n");

    if (request->HasString(K_FIELD_TYPES)) {
        auto mimeType = request->GetString(K_FIELD_TYPES, 0);
        printf("target requesting mime type: [%s]\n", mimeType);
        if (!strcmp(mimeType, B_FILE_MIME_TYPE)) {
            // target wants a file

            // get actual mime type
            auto fileMimeType = request->GetString(K_FIELD_FILETYPES, 0);
            printf("target requesting file mime type: [%s]\n", fileMimeType);

            entry_ref dirRef;
            const char *filename;
            if (request->FindRef("directory", &dirRef) == B_OK
                    && request->FindString("name", &filename) == B_OK)
            {
                BDirectory bDir(&dirRef);
                BFile outFile(&bDir, filename, B_WRITE_ONLY);
                finalizer->finalizeDrop(action, fileMimeType, &outFile);
                printf("wrote file for recipient!\n");

                // force mimetype on file because Tracker seems to set it inconsistently ...
                //  (eg. text/plain works but text/x-vnd.Be-stxt does not)
                BNodeInfo nodeInfo(&outFile);
                nodeInfo.SetType(fileMimeType);
            } else {
                printf("didn't find required directory + name in file request\n");
            }
        } else {
            // direct send via message
            BMallocIO outStream;
            outStream.SetBlockSize(16 * 1024);

            finalizer->finalizeDrop(action, mimeType, &outStream);
            reply.AddData(mimeType, B_MIME_DATA, outStream.Buffer(), outStream.BufferLength());

            request->SendReply(&reply);
            printf("reply sent!\n");
        }
    }

    // propagate action back to view, so it can update the status
}







#include "DNDEncoder.h"

#include <TranslationKit.h>
#include <MediaKit.h>
#include <SupportKit.h>

#include <stdio.h>
#include <assert.h>

#include "Globals.h"

class DropFinalizer {
public:
    virtual void finalizeDropAsData(int32 action, const char *mimeType, BMessage *outMsg) = 0;
    virtual void finalizeDropAsFile(int32 action, const char *mimeType, BFile *outFile) = 0;
};

class TextFinalizer : public DropFinalizer {
    char *data = nullptr;
    size_t dataSize = 0;
public:
    TextFinalizer(const char *dataToCopy, size_t dataSize)
        :dataSize(dataSize)
    {
        data = new char[dataSize];
        memcpy(data, dataToCopy, dataSize);
    }
    ~TextFinalizer() {
        if (data) {
            delete data;
        }
    }

    void finalizeDropAsFile(int32 action, const char *mimeType, BFile *outFile) override
    {
        switch(action) {
        case B_COPY_TARGET:
        case B_MOVE_TARGET:
        {
            if (!strcmp(mimeType, "text/plain")) {
                outFile->Write(data, dataSize);
                printf("TextFinalizer provided data!\n");
            } else if (!strcmp(mimeType, "text/x-vnd.Be-stxt")) {
                // perform conversion from B_TRANSLATOR_TEXT
                BMemoryIO inStream(data, dataSize);
                BMallocIO outStream;
                outStream.SetBlockSize(16 * 1024);
                auto roster = BTranslatorRoster::Default();
                if (roster->Translate(&inStream, nullptr, nullptr, &outStream, B_STYLED_TEXT_FORMAT) == B_OK) {
                    printf("successfully produced fancy text! woot!\n");
                    outFile->Write(outStream.Buffer(), outStream.BufferLength());
                } else {
                    printf("text finalizeDropAsFile roster->Translate failed\n");
                }
            } else {
                printf("text finalizeDropAsData - asked for unknown type [%s]\n", mimeType);
            }
            break;
        }
        default:
            printf("not sure what to do for link/trash yet\n");
        }
        // propagate action back to view, so it can update the status
    }

    void finalizeDropAsData(int32 action, const char *mimeType, BMessage *outMsg) override
    {
        switch(action) {
        case B_COPY_TARGET:
        case B_MOVE_TARGET:
        {
            if (!strcmp(mimeType, "text/plain")) {
                outMsg->AddData(mimeType, B_MIME_DATA, data, dataSize);
                printf("TextFinalizer provided data!\n");
            } else if (!strcmp(mimeType, "text/x-vnd.Be-stxt")) {
                // perform conversion from B_TRANSLATOR_TEXT
                BMemoryIO inStream(data, dataSize);
                BMallocIO outStream;
                outStream.SetBlockSize(16 * 1024);
                auto roster = BTranslatorRoster::Default();
                if (roster->Translate(&inStream, nullptr, nullptr, &outStream, B_STYLED_TEXT_FORMAT) == B_OK) {
                    printf("successfully produced fancy text! woot!\n");
                    outMsg->AddData(mimeType, B_MIME_DATA, outStream.Buffer(), outStream.BufferLength());
                } else {
                    printf("text finalizeDropAsData roster->Translate failed\n");
                }
            } else {
                printf("text finalizeDropAsData - asked for unknown type [%s]\n", mimeType);
            }
            break;
        }
        default:
            printf("not sure what to do for link/trash yet\n");
        }
        // propagate action back to view, so it can update the status
    }
};

// ===================================

status_t DNDEncoder::readFileData(BFile *file, char **buffer, off_t *length)
{
    if (file->GetSize(length) == B_OK) {
        file->Seek(SEEK_SET, 0);

        *buffer = new char[*length];
        file->Read(*buffer, *length);
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

void DNDEncoder::addTranslations(BPositionIO *source, const char *nativeMIME, const char *nativeDesc)
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

    // attempting to the use the Translation Kit to provide more text formats
    // ... doesn't do much (besides providing the "text/x-vnd.Be-stxt" format)
    file->Seek(0, SEEK_SET);
    BFile outStream("tmp/negotiated.txt", B_CREATE_FILE | B_READ_WRITE);
    auto roster = BTranslatorRoster::Default();
    if (roster->Translate(file, nullptr, nullptr, &outStream, B_TRANSLATOR_TEXT) == B_OK) {
        outStream.Seek(0, SEEK_SET);

        addTranslations(&outStream, "text/plain", "Plain text file");
    }

    // read the converted/normalized data
    if (readFileData(&outStream, &buffer, &length) == B_OK) {
        finalizer = new TextFinalizer(buffer, length);
        delete[] buffer;
    }
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
    // negotiated content
    // first convert into a 'baseline type' (from which all the other conversions are possible)
    BFile file(path, B_READ_ONLY);
    BBitmapStream stream;
    auto roster = BTranslatorRoster::Default();
    if (roster->Translate(&file, nullptr, nullptr, &stream, B_TRANSLATOR_BITMAP) == B_OK)
    {
        addTranslations(&stream, "image/x-be-bitmap", "Be Bitmap Format");
    }
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

const char *getActionString(int32 action)
{
    switch(action) {
    case B_COPY_TARGET:
        return "B_COPY_TARGET";
    case B_MOVE_TARGET:
        return "B_MOVE_TARGET";
    case B_LINK_TARGET:
        return "B_LINK_TARGET";
    case B_TRASH_TARGET:
        return "B_TRASH_TARGET";
    }
    return "(unknown)";
}

void DNDEncoder::finalizeDrop(BMessage *msg)
{
    BMessage reply(B_MIME_DATA);

    if (!finalizer) {
        printf("null finalizer in DNDEncoder::finalizeDrop! canceling\n");
        return;
    }

    // verify structure of msg etc
    auto action = msg->what;
    if (action != B_COPY_TARGET && action != B_MOVE_TARGET && action != B_LINK_TARGET && action != B_TRASH_TARGET) {
        printf("DND negotiation action (message->what) was not one of: \n");
        printf("  B_COPY_TARGET\n  B_MOVE_TARGET\n  B_LINK_TARGET\n  B_TRASH_TARGET");
        return;
    }
    printf("negotiated action is %s\n", getActionString(action));

    // direct message or file?
    // plus we need to verify that we even offered to send it that way ...
    printf("=======\n");
    msg->PrintToStream();
    printf("=======\n");

    if (msg->HasString(K_FIELD_TYPES)) {
        auto mimeType = msg->GetString(K_FIELD_TYPES, 0);
        printf("target requesting mime type: [%s]\n", mimeType);
        if (!strcmp(mimeType, B_FILE_MIME_TYPE)) {
            // target wants a file

            // get actual mime type
            auto fileMimeType = msg->GetString(K_FIELD_FILETYPES, 0);
            printf("target requesting file mime type: [%s]\n", fileMimeType);

            entry_ref dirRef;
            const char *filename;
            if (msg->FindRef("directory", &dirRef) == B_OK
                    && msg->FindString("name", &filename) == B_OK)
            {
                BDirectory bDir(&dirRef);
                BFile outFile(&bDir, filename, B_WRITE_ONLY);
                finalizer->finalizeDropAsFile(action, fileMimeType, &outFile);
                printf("wrote file for recipient!\n");

                // force mimetype on file because Tracker seems to set it inconsistently ...
                //  (text/plain works but text/x-vnd.Be-stxt does not)
                BNodeInfo nodeInfo(&outFile);
                nodeInfo.SetType(fileMimeType);
            } else {
                printf("didn't find required directory + name in file request\n");
            }
        } else {
            // direct send via message
            finalizer->finalizeDropAsData(action, mimeType, &reply);
            msg->SendReply(&reply);
            printf("reply sent!\n");
        }
    }
}







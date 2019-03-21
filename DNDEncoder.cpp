#include "DNDEncoder.h"

#include <TranslationKit.h>
#include <MediaKit.h>

#include <stdio.h>
#include <assert.h>

static const char *K_FIELD_CLIP_NAME = "be:clip_name";
static const char *K_FIELD_ACTIONS = "be:actions";
static const char *K_FIELD_ORIGINATOR = "be:originator";
static const char *K_FIELD_TYPES = "be:types";
static const char *K_FIELD_FILETYPES = "be:filetypes";
static const char *K_FIELD_TYPE_DESCS = "be:type_descriptions";

status_t DNDEncoder::readFileData(BFile *file, char **buffer, off_t *length)
{
    if (file->GetSize(length) == B_OK) {
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
    msg->AddPointer(K_FIELD_ORIGINATOR, this);
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

void DNDEncoder::addTextFormat(const char *text)\
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

    file->Seek(0, SEEK_SET);
    BFile outStream("tmp/negotiated.txt", B_CREATE_FILE | B_READ_WRITE);
    auto roster = BTranslatorRoster::Default();
    if (roster->Translate(file, nullptr, nullptr, &outStream, B_TRANSLATOR_TEXT) == B_OK) {
        outStream.Seek(0, SEEK_SET);
        addTranslations(&outStream, "text/plain", "Plain text file");
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







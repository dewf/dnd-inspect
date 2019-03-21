#pragma once

#include <StorageKit.h>

class DNDEncoder {
    BMessage *msg;

    status_t readFileData(BFile *file, char **buffer, off_t *length);
    void addFileRef(const char *path);
    void addTranslations(BPositionIO *source, const char *nativeMIME, const char *nativeDesc);
    void addNegotiatedPrologue();
public:
    DNDEncoder(BMessage *msg, const char *clipName);
    void addTextFormat(const char *text);
    void addTextFormat(BFile *file);
    void addColor(rgb_color color);
    void addFileList(const char *fnames[], int count);
    void addBitmap(const char *path);

    struct FileContent_t {
        const char *path;
        const char *mimeType;
        const char *desc;
    };
    void addFileContents(FileContent_t *files, int numFiles);

    BMessage *getMessage() { return msg; }
};

#pragma once

#include <StorageKit.h>

class DNDEncoder {
    BMessage *msg;

    status_t readFileData(BFile *file, char **buffer, off_t *length);
    void addFileRef(const char *path);
    void addTranslations(BPositionIO *source, const char *nativeMIME, const char *nativeDesc);
public:
    DNDEncoder(BMessage *msg, const char *clipName);
    void addTextFormat(const char *text);
    void addTextFormat(BFile *file);
    void addColor(rgb_color color);
    void addFileList(const char *fnames[], int count);
    void addMP3(const char *path);
    void addPNG(const char *path);
};

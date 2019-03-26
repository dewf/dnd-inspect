#pragma once

#include <InterfaceKit.h>
#include <StorageKit.h>

class DropFinalizer;

class DNDEncoder {
    DropFinalizer *finalizer = nullptr; // what to follow up with once the recipient chooses a format / action
    BMessage *msg = nullptr;

    status_t readFileData(BFile *file, char **buffer, off_t *length);
    void addFileRef(const char *path);
    void addNegotiatedPrologue();
public:
    struct FileContent_t {
        const char *path;
        const char *mimeType;
        const char *desc;
    };

    DNDEncoder(BMessage *msg, const char *clipName);
    ~DNDEncoder();
    BMessage *getMessage() { return msg; }

    void addTextFormat(const char *text);
    void addTextFormat(BFile *file);
    void addColor(rgb_color color);
    void addFileList(const char *fnames[], int count);
    void addBitmap(const char *path);
    void addFileContents(FileContent_t *files, int numFiles);

    // returns one of B_TARGET_COPY / _MOVE / etc
    enum DragResult : int32 {
        Failed,
        Copied = B_COPY_TARGET,
        Moved = B_MOVE_TARGET,
        Linked = B_LINK_TARGET,
        Trashed = B_TRASH_TARGET,
    };
    DragResult finalizeDrop(BMessage *msg); // handle the incoming negotiation message
};

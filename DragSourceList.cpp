#include "DragSourceList.h"

#include <stdio.h>
#include <cstring>

#include <Screen.h>
#include <Bitmap.h>
#include <algorithm>

#include <StorageKit.h>

static const char *K_FIELD_CLIP_NAME = "be:clip_name";
static const char *K_FIELD_ACTIONS = "be:actions";

class DNDEncoder {
    BMessage *msg;

    status_t readFileData(BFile *file, char **buffer, off_t *length) {
        if (file->GetSize(length) == B_OK) {
            *buffer = new char[*length];
            file->Read(*buffer, *length);
            return B_OK;
        } else {
            return B_ERROR;
        }
    }

    void addFileRef(const char *path) {
        BEntry entry(path);
        entry_ref ref;
        if (entry.GetRef(&ref) == B_OK) {
            msg->AddRef("refs", &ref);
        }
    }
public:
    DNDEncoder(BMessage *msg, const char *clipName)
        :msg(msg)
    {
        msg->AddString(K_FIELD_CLIP_NAME, clipName);
        msg->AddInt32(K_FIELD_ACTIONS, B_COPY_TARGET);
        msg->AddInt32(K_FIELD_ACTIONS, B_MOVE_TARGET);
        msg->AddInt32(K_FIELD_ACTIONS, B_TRASH_TARGET);
        msg->AddInt32(K_FIELD_ACTIONS, B_LINK_TARGET);
    }
    void addTextFormat(const char *text) {
        msg->SetData("text/plain", B_MIME_DATA, text, strlen(text));
    }
    void addTextFormat(BFile *file) {
        char *buffer;
        off_t length;
        if (readFileData(file, &buffer, &length) == B_OK) {
            msg->SetData("text/plain", B_MIME_DATA, buffer, length);
            delete[] buffer;
        }
    }
    void addColor(rgb_color color) {
        msg->SetData("RGBColor", B_RGB_COLOR_TYPE, &color, sizeof(color));
    }

    void addFileList(const char *fnames[], int count) {
        for (int i=0; i< count; i++) {
            addFileRef(fnames[i]);
        }
    }
    void addMP3(const char *path) {
        addFileRef(path);
    }
    void addPNG(const char *path) {
        addFileRef(path);
    }
};

struct formatItem_t {
    const char *label;
    void (*createMsg)(DNDEncoder *encoder);
};

static void shortTextFormat(DNDEncoder *encoder) {
    encoder->addTextFormat("HERE IS SOME TEXT AND STUFF, NOICE");
}

static void colorFormat(DNDEncoder *encoder) {
    encoder->addColor(rgb_color{0, 120, 255, 255});
}

static void longTextFormat(DNDEncoder *encoder) {
    BFile loremIpsum("_testcontent/lorem_ipsum.txt", B_READ_ONLY);
    encoder->addTextFormat(&loremIpsum);
}

static void multipleFilesFormat(DNDEncoder *encoder) {
    const char *fnames[4] = {
                   "_testcontent/bachfugue.mp3",
                   "_testcontent/be_icons.png",
                   "_testcontent/big_buck_bunny.mp4",
                   "_testcontent/lorem_ipsum.txt" };

    encoder->addFileList(fnames, 4);
}

static void mp3DataFormat(DNDEncoder *encoder) {
    encoder->addMP3("_testcontent/bachfugue.mp3");
}

static void pngDataFormat(DNDEncoder *encoder) {
    encoder->addPNG("_testcontent/be_icons.png");
}

formatItem_t items[] {
    {"short text", shortTextFormat},
    {"RGB color", colorFormat},
    {"long text", longTextFormat},
    {"multiple file refs", multipleFilesFormat},
    {"mp3 data", mp3DataFormat},
    {"png data", pngDataFormat},
};

DragSourceList::DragSourceList(BRect r)
    :BListView(r, "list", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL)
{
    auto count = sizeof(items) / sizeof(formatItem_t);
    for (int i=0; i< count; i++) {
        AddItem(new BStringItem(items[i].label));
    }
}

void DragSourceList::MouseDown(BPoint where)
{
    BListView::MouseDown(where); // update selection
    if (CurrentSelection() >= 0) {
        auto frame = ItemFrame(CurrentSelection());
        mouseGrabOffs = where - frame.LeftTop();
    }
}

BRect intersectRects(BRect a, BRect b)
{
    return BRect(std::max(a.left, b.left),
                 std::max(a.top, b.top),
                 std::min(a.right, b.right),
                 std::min(a.bottom, b.bottom));
}

bool DragSourceList::InitiateDrag(BPoint point, int32 index, bool wasSelected)
{
    BMessage bMsg(B_SIMPLE_DATA);
    DNDEncoder encoder(&bMsg, items[index].label);
    items[index].createMsg(&encoder); // populate the message with the item-specific data

    // move frame to where it needs to be
    BPoint mousePos;
    uint32 buttons;
    GetMouse(&mousePos, &buttons);

//    auto frame = ItemFrame(index);
//    frame.OffsetTo(mousePos - mouseGrabOffs);
//    DragMessage(&bMsg, frame);

    // get the bitmap for this item
    BScreen screen(Window());
    auto itemFrame = ItemFrame(index);
    ConvertToScreen(&itemFrame);
    auto isected = intersectRects(screen.Frame(), itemFrame);

    auto bitmapBounds = BRect(0, 0, isected.Width(), isected.Height());
    auto bitmap = new BBitmap(bitmapBounds, screen.ColorSpace());
    screen.ReadBitmap(bitmap, false, &isected);

    DragMessage(&bMsg, bitmap, mouseGrabOffs);
    return true;
}

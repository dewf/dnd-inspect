#include "DragSourceList.h"

#include <stdio.h>

#include <Screen.h>
#include <Bitmap.h>
#include <algorithm>

#include "DNDEncoder.h"

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

static void audioFormat(DNDEncoder *encoder) {
    encoder->addMP3("_testcontent/bachfugue.mp3");
}

static void bitmapFormat(DNDEncoder *encoder) {
    encoder->addPNG("_testcontent/be_icons.png");
}

formatItem_t items[] {
    {"short text", shortTextFormat},
    {"RGB color", colorFormat},
    {"multiple file refs", multipleFilesFormat},
    {"long text (simple + negotiated)", longTextFormat},
    {"bitmap data (ref + negotiated)", bitmapFormat},
    {"audio data (negotiated)", audioFormat},
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

#pragma once

#include <ListView.h>

class DNDEncoder;

class DragSourceList : public BListView
{
    BPoint mouseGrabOffs;
    DNDEncoder *encoder = nullptr;

    void MessageReceived(BMessage *message) override;
public:
    DragSourceList(BRect r);

    void MouseDown(BPoint where) override;
    bool InitiateDrag(BPoint point, int32 index, bool wasSelected) override;
};

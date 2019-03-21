#include "DropTargetView.h"

#include <InterfaceKit.h>

#include <stdio.h>
#include <cstring>

class DroppableTextView : public BTextView
{
public:
    DroppableTextView(BRect r, const char *name)
        :BTextView(r, name, r.InsetByCopy(2, 2), B_FOLLOW_ALL) {}

    void MessageReceived(BMessage *msg) override {
        switch(msg->what) {
        case B_SIMPLE_DATA:
        {
            char buffer[4096];
            snprintf(buffer, 4096, "Got a new DnD drop!\n");
            Insert(buffer);
            break;
        }
        default:
            BTextView::MessageReceived(msg);
        }
    }
};


DropTargetView::DropTargetView(BRect r)
    :BView(r, "droptarget", B_FOLLOW_ALL, 0)
{
    AdoptSystemColors();

    //r.InsetBy(10, 10);
    auto logArea = new DroppableTextView(r, "log");
    AddChild(logArea);
}

#include "DropTargetView.h"

#include <InterfaceKit.h>

#include <stdio.h>
#include <cstring>

#include "Globals.h"

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

            if (msg->HasInt32(K_FIELD_ACTIONS)) {
                snprintf(buffer, 4096, "Negotiated drop detected\n");

                // just choose the first action for now
                auto action = msg->GetInt32(K_FIELD_ACTIONS, 0, B_COPY_TARGET);
                auto firstType = msg->GetString(K_FIELD_TYPES, 0, nullptr);

                BMessage reply(action);
                reply.AddString(K_FIELD_TYPES, firstType);
                msg->SendReply(&reply);
            } else {
                // simple drop
                snprintf(buffer, 4096, "Simple drop detected\n");
            }
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

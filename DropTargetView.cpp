#include "DropTargetView.h"

#include <InterfaceKit.h>

#include <stdio.h>
#include <cstring>

#include "Globals.h"

#include "DropDialog.h"

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

                auto dd = new DropDialog(Window(), msg);
                BMessage *negotiationMsg;
                if (dd->GenerateResponse(&negotiationMsg)) {

                    // need to add our window to negotiation msg,
                    //  because the application object is receiving the final B_MIME_DATA for some reason
                    // but we want to process it on this thread :(
                    auto dropWindow = Window();
                    negotiationMsg->AddPointer(K_FIELD_DROPWINDOW, dropWindow);
                    printf("## dropwindow ptr: %08X\n", dropWindow);

                    // send from this thread, if that matters
                    msg->SendReply(negotiationMsg);

                    printf("== sent negotiation msg\n");
                    delete negotiationMsg;
                }
                // woot
                printf("GenerateResponse returned\n");
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

    logArea = new DroppableTextView(r, "log");
    AddChild(logArea);
}

void DropTargetView::processFinalDrop(BMessage *msg)
{
    logArea->Insert("Fucking FINALLY\n");
}

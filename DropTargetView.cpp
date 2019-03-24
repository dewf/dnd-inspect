#include "DropTargetView.h"

#include <InterfaceKit.h>

#include <stdio.h>
#include <cstring>

#include "Globals.h"

#include "DropDialog.h"

#include <assert.h>


class DroppableTextView : public BTextView
{
    static const size_t LOG_BUFFER_SIZE = 64 * 1024;
    char buffer[LOG_BUFFER_SIZE];
public:
    DroppableTextView(BRect r, const char *name)
        :BTextView(r, name, r.InsetByCopy(2, 2), B_FOLLOW_ALL) {}

    void logPrintf(const char *format, ...) {
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, LOG_BUFFER_SIZE, format, args);
        Insert(buffer);
        va_end(args);
    }

    void MessageReceived(BMessage *msg) override {
        switch(msg->what) {
        case B_SIMPLE_DATA:
        {
            logPrintf("==================================\n");
            if (msg->HasInt32(K_FIELD_ACTIONS)) {
                logPrintf("Negotiated drop detected\n");

                auto dd = new DropDialog(Window(), msg);
                BMessage *negotiationMsg;
                if (dd->GenerateResponse(&negotiationMsg)) {

                    // need to add our window to negotiation msg,
                    //  because the application object is receiving the final B_MIME_DATA for some reason
                    // but we want to process it on this thread :(
                    auto dropWindow = Window();
                    negotiationMsg->AddPointer(K_FIELD_DROPWINDOW, dropWindow);
                    logPrintf("## dropwindow ptr: %08X\n", dropWindow);

                    // send from this thread, if that matters
                    msg->SendReply(negotiationMsg);

                    logPrintf("== sent negotiation msg\n");
                    delete negotiationMsg;
                }
                // woot
                logPrintf("GenerateResponse returned\n");
            } else {
                // simple drop
                logPrintf("Simple drop detected\n");
            }
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
    if (msg->IsReply()) {
        auto prev = msg->Previous();
        assert(prev != nullptr); // for some reason msg->IsReply() was not working?
        logArea->logPrintf("isreply: %s\n", msg->IsReply() ? "true" : "false");

        auto mimeType = prev->GetString(K_FIELD_TYPES); // the type we originally requested
        logArea->logPrintf("Received direct data drop [%s]", mimeType);

        type_code typeCode;
        const void *data;
        ssize_t numBytes;
        if (msg->FindData(mimeType, B_MIME_DATA, &data, &numBytes) == B_OK) {
            logArea->logPrintf("= payload size: %zu\n", numBytes);
        } else {
            logArea->logPrintf("uuhh something doesn't match up\n");
        }
    }
}

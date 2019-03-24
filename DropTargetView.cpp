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

    bool dumpMimeData(BMessage *msg) {
        char *mimeType;
        type_code typeFound;
        if (msg->GetInfo(B_MIME_DATA, 0, &mimeType, &typeFound) == B_OK) {
            logPrintf("mime type [%s]\n", mimeType);
            const void *data;
            ssize_t numBytes;
            if (msg->FindData(mimeType, B_MIME_DATA, &data, &numBytes) == B_OK) {
                logPrintf("mime data: %zd bytes\n", numBytes);
                if (!strcmp(mimeType, "text/plain")) {
                    auto terminated = new char[numBytes];
                    strncpy(terminated, (const char *)data, numBytes);
                    terminated[numBytes] = 0;
                    logPrintf(" - text: [%s]\n", terminated);
                    delete[] terminated;
                }
                return true;
            }
        }
        return false;
    }

    bool dumpColorData(BMessage *msg) {
        const void *data;
        ssize_t numBytes;
        if (msg->FindData("RGBColor", B_RGB_COLOR_TYPE, &data, &numBytes) == B_OK) {
            auto c = *((rgb_color *)data);
            logPrintf("RGBColor: %02X.%02X.%02X.%02X\n", c.red, c.green, c.blue, c.alpha);
            return true;
        }
        return false;
    }

    void dumpSimpleDrop(BMessage *msg) {
        // check for RGB colors, file refs, etc
        if (!dumpMimeData(msg))
            if (!dumpColorData(msg)) {}
    }

    void MessageReceived(BMessage *msg) override {
        switch(msg->what) {
        case B_SIMPLE_DATA:
        {
            logPrintf("==================================\n");
            if (msg->HasInt32(K_FIELD_ACTIONS)) {
                logPrintf("Negotiated drop detected\n");

                // current dialog implementation is blocking
                // alternatively we could detach the message(?)
                //  and let the dialog run merrily in its own thread,
                //  that doesn't seem to present any issue as far as how this works
                auto dd = new DropDialog(Window(), msg);
                BMessage *negotiationMsg;
                if (dd->GenerateResponse(&negotiationMsg)) {
                    // need to add our window to negotiation msg,
                    //  because the application object is receiving the final B_MIME_DATA for some reason
                    // but we want to process it on this thread,
                    //  hence the need to provide this window to send that message to (see App.cpp)
                    auto dropWindow = Window();
                    negotiationMsg->AddPointer(K_FIELD_DROPWINDOW, dropWindow);

                    // send from this thread (vs. dialog thread), if that matters
                    msg->SendReply(negotiationMsg);

                    delete negotiationMsg;
                } else {
                    logPrintf("(drop canceled)\n");
                }
            } else {
                // simple drop
                logPrintf("Simple drop detected\n");
                dumpSimpleDrop(msg);
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

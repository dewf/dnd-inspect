#include "DropTargetView.h"

#include <LayoutBuilder.h>
#include <InterfaceKit.h>
#include <StorageKit.h>

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
    DroppableTextView(const char *name)
        :BTextView(name) {
		BRect rect = Bounds();
		SetTextRect(rect.InsetByCopy(7, 1));
	}

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
        if (msg->FindData(K_FIELD_RGBCOLOR, B_RGB_COLOR_TYPE, &data, &numBytes) == B_OK) {
            auto c = *((rgb_color *)data);
            logPrintf("RGBColor: %02X.%02X.%02X.%02X\n", c.red, c.green, c.blue, c.alpha);
            return true;
        }
        return false;
    }

    bool dumpFileRefs(BMessage *msg) {
        type_code typeCode;
        int32 countFound;
        if (msg->GetInfo(K_FIELD_REFS, &typeCode, &countFound) == B_OK
                && typeCode == B_REF_TYPE)
        {
            logPrintf("%d file refs:\n", countFound);
            for (int i=0; i< countFound; i++) {
                entry_ref ref;
                if (msg->FindRef(K_FIELD_REFS, i, &ref) == B_OK) {
                    BPath path(&ref);
                    logPrintf(" - %s\n", path.Path());
                }
            }
            return true;
        }
        return false;
    }

    void dumpSimpleDrop(BMessage *msg) {
        // check for RGB colors, file refs, etc
        if (!dumpMimeData(msg))
            if (!dumpColorData(msg))
                if (!dumpFileRefs(msg)) {}
    }

    void MessageReceived(BMessage *msg) override {
        switch(msg->what) {
        case B_SIMPLE_DATA:
        {
            SelectAll();
            Clear();

            if (msg->HasInt32(K_FIELD_ACTIONS)) {
                logPrintf("Negotiated drop detected\n");

                // current dialog implementation is blocking
                // alternatively we could detach the message(?)
                //  and let the dialog run merrily in its own thread.
                // that doesn't seem to present any issue as far as how this works
                auto dd = new DropDialog(Window(), msg);
                BMessage *negotiationMsg;
                if (dd->GenerateResponse(&negotiationMsg)) {

                    msg->SendReply(negotiationMsg, this);

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
        case B_MIME_DATA: // final drop
        {
            if (msg->IsReply()) {
                auto prev = msg->Previous();

                auto mimeType = prev->GetString(K_FIELD_TYPES); // the type we originally requested
                logPrintf("Receiving direct data drop - expecting [%s] ...\n", mimeType);

                type_code typeCode;
                const void *data;
                ssize_t numBytes;
                if (msg->FindData(mimeType, B_MIME_DATA, &data, &numBytes) == B_OK) {
                    logPrintf(" = OK! payload size: %zu\n", numBytes);
                } else {
                    logPrintf(" = ERROR! didn't find the mime type we asked for [%s] in the received B_MIME_DATA message\n", mimeType);
                }
            }
            break;
        }
        default:
            BTextView::MessageReceived(msg);
        }
    }
};

DropTargetView::DropTargetView()
    :BView("droptarget", B_WILL_DRAW, new BGroupLayout(B_VERTICAL, B_USE_DEFAULT_SPACING))
{
    AdoptSystemColors();

    logArea = new DroppableTextView("log");
    auto scroll = new BScrollView("scrollview", logArea, B_WILL_DRAW, false, true);

    AddChild(scroll);
}

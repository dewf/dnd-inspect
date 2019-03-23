#include "App.h"

#include <InterfaceKit.h>

#include <stdio.h>
#include "Globals.h"

void App::MessageReceived(BMessage *msg)
{
    switch(msg->what) {
    case B_MIME_DATA:
        // route to the view that sent to B_xxxx_TARGET
        if (msg->IsReply()) {
            msg->PrintToStream();
            auto prev = msg->Previous();
            auto dropWindow = (BWindow *)prev->GetPointer(K_FIELD_DROPWINDOW);
            printf("dropwindow ptr: %08X\n", dropWindow);
            BMessage msg2(*msg); // copy it - necessary?
            dropWindow->PostMessage(&msg2);
            printf("but we seeeent it :((\n");
        }
        break;
    default:
        BApplication::MessageReceived(msg);
    }
}

App::App()
    :BApplication("application/x-vnd.nobody-DnDInspector")
{
    // woot
}

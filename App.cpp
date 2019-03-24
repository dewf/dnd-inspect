#include "App.h"

#include <InterfaceKit.h>

#include <stdio.h>
#include "Globals.h"

void App::MessageReceived(BMessage *msg)
{
    switch(msg->what) {
    case B_MIME_DATA:
        // route to the view that sent to B_xxxx_TARGET
        // don't know why it comes to the app and not the window ...
        if (msg->IsReply()) {
            auto prev = msg->Previous();
            auto dropWindow = (BWindow *)prev->GetPointer(K_FIELD_DROPWINDOW);
            dropWindow->PostMessage(msg);
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

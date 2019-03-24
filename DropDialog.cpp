#include "DropDialog.h"

#include <GroupLayout.h>
#include <MimeType.h>

#include "Globals.h"

#include <assert.h>
#include <stdio.h>

enum IntConstants : int32 {
    // message 'what's
    K_ACTION_MENU_SELECT = 'kX@1', // some random gibberish unlikely to collide
    K_TYPES_MENU_SELECT,
    K_FILETYPES_MENU_SELECT,
    K_OK_PRESSED,
    K_DROP_AS_FILE_CHECK,

    // thread notify code
    K_THREAD_NOTIFY
};

BRect centeredRect(BWindow *centerOn, int width, int height)
{
    auto winScreenBounds = centerOn->ConvertToScreen(centerOn->Bounds());
    auto cx = winScreenBounds.left + (winScreenBounds.Width() - width) / 2;
    auto cy = winScreenBounds.top + (winScreenBounds.Height() - height) / 2;
    return BRect(cx, cy, cx + width, cy + height);
}

DropDialog::DropDialog(BWindow *centerOn, BMessage *dropMsg)
    :BWindow(centeredRect(centerOn, 400, 400), "Drop Parameters", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, 0),
      dropMsg(dropMsg)
{
    auto layout = new BGroupLayout(B_VERTICAL, 0);
    SetLayout(layout);

    // file or direct drop
    type_code typesTypeCode;
    int32 numTypesFound;
    bool typesFixedSize;
    if (dropMsg->GetInfo(K_FIELD_TYPES, &typesTypeCode, &numTypesFound, &typesFixedSize) == B_OK
            && numTypesFound > 0 && typesTypeCode == B_STRING_TYPE)
    {
        auto typesMenu = new BPopUpMenu("(choose drop data type)");

        bool hasFileTypes = false;

        for(int i=0; i< numTypesFound; i++) {
            auto _type = dropMsg->GetString(K_FIELD_TYPES, i, "x");

            if (!strcmp(_type, B_FILE_MIME_TYPE)) {
                hasFileTypes = true;
                if (i != numTypesFound - 1) {
                    printf("note: B_FILE_MIME_TYPE should only appear last in the be:types list\n");
                }
            } else {
                // non-file type
                auto msg = new BMessage(K_TYPES_MENU_SELECT);
                msg->SetString(K_FIELD_DEFAULT, _type);
                typesMenu->AddItem(new BMenuItem(_type, msg));
            }
        }
        typeChooser = new BMenuField("Data Type", typesMenu);
        layout->AddView(typeChooser);

        // if last is filetype, check those
        if (hasFileTypes) {
            // checkbox to enable file types
            dropAsFile = new BCheckBox("Drop as file", new BMessage(K_DROP_AS_FILE_CHECK));
            layout->AddView(dropAsFile);

            type_code fileTypesTypeCode;
            int32 numFileTypesFound;
            bool fileTypesFixedSize;
            if (dropMsg->GetInfo(K_FIELD_FILETYPES, &fileTypesTypeCode, &numFileTypesFound, &fileTypesFixedSize) == B_OK
                    && numFileTypesFound > 0 && fileTypesTypeCode == B_STRING_TYPE)
            {
                auto fileTypesMenu = new BPopUpMenu("(choose file data type)");

                for(int i=0; i< numFileTypesFound; i++) {
                    auto fileType = dropMsg->GetString(K_FIELD_FILETYPES, i, "x");

                    auto msg = new BMessage(K_FILETYPES_MENU_SELECT);
                    msg->SetString(K_FIELD_DEFAULT, fileType);

                    fileTypesMenu->AddItem(new BMenuItem(fileType, msg));
                }
                fileTypeChooser = new BMenuField("File Type", fileTypesMenu);
                fileTypeChooser->SetEnabled(false); // disabled until user chooses "drop as file" in data type above
                layout->AddView(fileTypeChooser);
            } else {
                printf("drop message doesn't contain [%s] field of type B_STRING_TYPE", K_FIELD_FILETYPES);
                PostMessage(B_QUIT_REQUESTED);
                return;
            }
        }
    } else {
        printf("drop message doesn't contain [%s] field of type B_STRING_TYPE", K_FIELD_TYPES);
        PostMessage(B_QUIT_REQUESTED);
        return;
    }


    // populate the drop action menu
    type_code actionsTypeCode;
    int32 numActionsFound;
    bool actionsFixedSize;
    if (dropMsg->GetInfo(K_FIELD_ACTIONS, &actionsTypeCode, &numActionsFound, &actionsFixedSize) == B_OK
            && numActionsFound > 0 && actionsTypeCode == B_INT32_TYPE)
    {
        auto actionMenu = new BPopUpMenu("(choose drop action)");

        for(int i=0; i< numActionsFound; i++) {
            auto action = dropMsg->GetInt32(K_FIELD_ACTIONS, i, 0);

            auto msg = new BMessage(K_ACTION_MENU_SELECT);
            msg->SetInt32(K_FIELD_DEFAULT, action);

            auto actionStr = getActionString(action);
            actionMenu->AddItem(new BMenuItem(actionStr, msg));
        }
        auto actionChooser = new BMenuField("Drop Action", actionMenu);
        layout->AddView(actionChooser);
    } else {
        printf("drop message doesn't contain [%s] field of type B_INT32_TYPE", K_FIELD_ACTIONS);
        PostMessage(B_QUIT_REQUESTED);
        return;
    }

    // cancel / go buttons
    auto buttonsLayout = new BGroupLayout(B_HORIZONTAL);
    layout->AddItem(buttonsLayout);

    auto cancelButton = new BButton("Cancel", new BMessage(B_QUIT_REQUESTED));
    auto dropButton = new BButton("Drop", new BMessage(K_OK_PRESSED));
    buttonsLayout->AddView(cancelButton);
    buttonsLayout->AddView(dropButton);
}

bool DropDialog::GenerateResponse(BMessage **negotiationMsg)
{
    waitingThread = find_thread(nullptr);
    Show();

    thread_id sender;
    while(receive_data(&sender, nullptr, 0) != K_THREAD_NOTIFY) {
        printf("drop dialog - ShowAndWait received spurious msg\n");
    }

    *negotiationMsg = this->negotiationMsg;
    return (this->negotiationMsg != nullptr);
}

void DropDialog::MessageReceived(BMessage *msg)
{
    switch(msg->what) {
    case K_ACTION_MENU_SELECT:
    {
        dropAction = msg->GetInt32(K_FIELD_DEFAULT, B_COPY_TARGET);
        break;
    }
    case K_TYPES_MENU_SELECT:
    {
        selectedType = msg->GetString(K_FIELD_DEFAULT);
        break;
    }
    case K_DROP_AS_FILE_CHECK:
    {
        auto state = dropAsFile->Value() == B_CONTROL_ON;
        typeChooser->SetEnabled(!state); // disable (or enable) normal types
        fileTypeChooser->SetEnabled(state); // enalbe (or disable) file types
        break;
    }
    case K_FILETYPES_MENU_SELECT:
    {
        selectedFileType = msg->GetString(K_FIELD_DEFAULT);
        break;
    }
    case K_OK_PRESSED:
    {
        printf("OK pressed\n");
        if (strcmp(selectedType.c_str(), B_FILE_MIME_TYPE) != 0) { // don't deal with files yet

            // construct the message to send (will be returned to dialog caller)
            negotiationMsg = new BMessage(dropAction);
            negotiationMsg->SetString(K_FIELD_TYPES, selectedType.c_str());

            // close the dialog
            PostMessage(B_QUIT_REQUESTED);
        }
        break;
    }
    default:
        BWindow::MessageReceived(msg);
    }
}

bool DropDialog::QuitRequested()
{
    // notify the caller of GenerateResponse() that we're done (whether by OK or Cancel)
    send_data(waitingThread, K_THREAD_NOTIFY, nullptr, 0);

    return true; // yes, please close window / end thread
}


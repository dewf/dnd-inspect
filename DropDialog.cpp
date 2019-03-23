#include "DropDialog.h"

#include <GroupLayout.h>
#include <MimeType.h>

#include "Globals.h"

#include <assert.h>
#include <stdio.h>

static const int32 K_ACTION_MENU_SELECT = 'AcMS';
static const int32 K_TYPES_MENU_SELECT = 'TyMS';
static const int32 K_FILETYPES_MENU_SELECT = 'FTMS';

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
    auto layout = new BGroupLayout(B_VERTICAL);
    SetLayout(layout);

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

            auto msg = new BMessage(K_TYPES_MENU_SELECT);
            msg->SetString(K_FIELD_DEFAULT, _type);

            if (i == (numTypesFound - 1) && !strcmp(_type, B_FILE_MIME_TYPE)) {
                hasFileTypes = true;
                typesMenu->AddItem(new BMenuItem("====== Drop as File ======", msg));
            } else {
                typesMenu->AddItem(new BMenuItem(_type, msg));
            }
        }
        typeChooser = new BMenuField("Data Type", typesMenu);
        layout->AddView(typeChooser);

        // if last is filetype, check those
        if (hasFileTypes) {
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
}

void DropDialog::MessageReceived(BMessage *msg)
{
    switch(msg->what) {
    case K_ACTION_MENU_SELECT:
    {
        dropAction = msg->GetInt32(K_FIELD_DEFAULT, B_COPY_TARGET);
        printf("new drop action: %08X\n", dropAction);
        break;
    }
    case K_TYPES_MENU_SELECT:
    {
        selectedType = msg->GetString(K_FIELD_DEFAULT);
        auto fileChooserEnabled = !strcmp(selectedType, B_FILE_MIME_TYPE);
        fileTypeChooser->SetEnabled(fileChooserEnabled);
        break;
    }
    case K_FILETYPES_MENU_SELECT:
    {
        selectedFileType = msg->GetString(K_FIELD_DEFAULT);
        break;
    }
    default:
        BWindow::MessageReceived(msg);
    }
}

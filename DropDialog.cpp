#include "DropDialog.h"

#include <LayoutBuilder.h>
#include <GroupLayout.h>
#include <CardLayout.h>
#include <MimeType.h>
#include <StorageKit.h>

#include <SpaceLayoutItem.h>

#include "Globals.h"

#include <assert.h>
#include <stdio.h>

enum IntConstants : int32 {
    // message 'what's
    K_TYPES_MENU_SELECT = 'kX@1', // some random gibberish unlikely to collide
    K_FILETYPES_MENU_SELECT,
    K_OK_PRESSED,
    K_DROP_AS_FILE_CHECK,
    K_OPEN_FILE_CHOOSER,
    K_FILE_PANEL_DONE,

    K_CLICKED_ACTION,

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

BButton *addActionButton(BGroupLayout *hbox, const char *label, int32 action)
{
    auto msg = new BMessage(K_CLICKED_ACTION);
    msg->SetInt32(K_FIELD_DEFAULT, action);
    auto button = new BButton(label, msg);
    button->SetEnabled(false);
    hbox->AddView(button);
    return button;
}

DropDialog::DropDialog(BWindow *centerOn, BMessage *dropMsg)
    :BWindow(centeredRect(centerOn, 350, 200), "Drop Parameters", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, 0),
      dropMsg(dropMsg)
{
    typesMenu = new BPopUpMenu("(choose drop data type)");
    typeChooser = new BMenuField("Data Type", typesMenu);

    //dropAsFile = new BCheckBox("Drop as file", new BMessage(K_DROP_AS_FILE_CHECK));

    fileTypesMenu = new BPopUpMenu("(choose file data type)");
    fileTypeChooser = new BMenuField("File Type", fileTypesMenu);

    // choose file button
    chooseFileButton = new BButton("Pick...\n", new BMessage(K_OPEN_FILE_CHOOSER));
    chosenPathLabel = new BStringView("path-label", "(no file selected)");

    actionMenu = new BPopUpMenu("(choose drop action)");
    actionChooser = new BMenuField("Drop Action", actionMenu);

    // cancel / go buttons
    auto buttonsLayout = new BGroupLayout(B_HORIZONTAL);
    auto cancelButton = new BButton("Cancel", new BMessage(B_QUIT_REQUESTED));
    auto dropButton = new BButton("Drop", new BMessage(K_OK_PRESSED));

    // layout ========================================
    auto group = new BGroupLayout(B_VERTICAL);
    SetLayout(group);
    //group->SetInsets(0);

    // tab area for data/file drop choice
    tabView = new BTabView("tabview");

    auto r = tabView->Bounds();
    r.bottom -= tabView->TabHeight(); // essentially adjusting the height, not bottom per se

    // data drop tab
    auto dataDropView = new BGroupView(B_VERTICAL);
    dataDropView->GroupLayout()->SetInsets(4);
    dataDropView->AddChild(typeChooser);
    dataDropView->AddChild(BSpaceLayoutItem::CreateGlue());

    auto hbox1 = new BGroupLayout(B_HORIZONTAL);
    dataDropView->AddChild(hbox1);
    dataActionButtons.copy = addActionButton(hbox1, "Copy", B_COPY_TARGET);
    dataActionButtons.move = addActionButton(hbox1, "Move", B_MOVE_TARGET);
    dataActionButtons.link = addActionButton(hbox1, "Link", B_LINK_TARGET);
    dataActionButtons.trash = addActionButton(hbox1, "Trash", B_TRASH_TARGET);

    dataTab = new BTab();
    tabView->AddTab(dataDropView, dataTab);
    dataTab->SetLabel("Data Drop");

    // file drop tab
    auto fileDropView = new BGroupView(B_VERTICAL);
    fileDropView->GroupLayout()->SetInsets(4);
    fileDropView->AddChild(fileTypeChooser);
       auto hbox = new BGroupView(B_HORIZONTAL);
       hbox->AddChild(BSpaceLayoutItem::CreateGlue());
       hbox->AddChild(chosenPathLabel);
       hbox->AddChild(chooseFileButton);
    fileDropView->AddChild(hbox);
    fileDropView->AddChild(BSpaceLayoutItem::CreateGlue());

    auto hbox2 = new BGroupLayout(B_HORIZONTAL);
    fileDropView->AddChild(hbox2);
    fileActionButtons.copy = addActionButton(hbox2, "Copy", B_COPY_TARGET);
    fileActionButtons.move = addActionButton(hbox2, "Move", B_MOVE_TARGET);
    fileActionButtons.link = addActionButton(hbox2, "Link", B_LINK_TARGET);
    fileActionButtons.trash = addActionButton(hbox2, "Trash", B_TRASH_TARGET);

    fileTab = new BTab();
    tabView->AddTab(fileDropView, fileTab);
    fileTab->SetLabel("File Drop");

    //
    group->AddView(tabView);

    // now actually fill and config those controls based on what was in the message
    configure();
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
    case K_TYPES_MENU_SELECT:
    {
        selectedType = msg->GetString(K_FIELD_DEFAULT);
        break;
    }
    case K_FILETYPES_MENU_SELECT:
    {
        selectedFileType = msg->GetString(K_FIELD_DEFAULT);
        break;
    }
    case K_CLICKED_ACTION:
    {
        printf("k-clicked-action\n");

        // construct the message to send (will be returned to dialog caller)
        auto dropAction = msg->GetInt32(K_FIELD_DEFAULT, B_COPY_TARGET);
        negotiationMsg = new BMessage(dropAction);

        auto selected = tabView->TabAt(tabView->Selection());
        if (selected == dataTab) {
            negotiationMsg->AddString(K_FIELD_TYPES, selectedType.c_str());
        } else {
            negotiationMsg->AddString(K_FIELD_TYPES, B_FILE_MIME_TYPE);
            negotiationMsg->AddString(K_FIELD_FILETYPES, selectedFileType.c_str());
            negotiationMsg->AddRef("directory", &chosenDir);
            negotiationMsg->AddString("name", chosenFilename.c_str());
            negotiationMsg->PrintToStream();
        }
        // close the dialog
        PostMessage(B_QUIT_REQUESTED);
        break;
    }
    case K_OPEN_FILE_CHOOSER:
    {
        filePanel = new BFilePanel(
                    B_SAVE_PANEL,
                    new BMessenger(this), // msg target
                    nullptr,
                    0,     // n/a
                    false, // multiple selection
                    new BMessage(K_FILE_PANEL_DONE),
                    nullptr,
                    true, // modal
                    true); // hide when done
        filePanel->Show();

        break;
    }
    case K_FILE_PANEL_DONE:
    {
        // file chosen
        printf("file selected\n");
        if (msg->FindRef("directory", &chosenDir) == B_OK) {
            chosenFilename = msg->GetString("name");
            // update label
            BPath path(new BDirectory(&chosenDir), chosenFilename.c_str());
            chosenPathLabel->SetText(path.Path());
            // enable the buttons that need to be enabled ...
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

void DropDialog::configure()
{
    // file or direct drop
    type_code typesTypeCode;
    int32 numTypesFound;
    bool typesFixedSize;
    if (dropMsg->GetInfo(K_FIELD_TYPES, &typesTypeCode, &numTypesFound, &typesFixedSize) == B_OK
            && numTypesFound > 0 && typesTypeCode == B_STRING_TYPE)
    {
        bool hasFileTypes = false;
        bool hasDataTypes = false;

        // get the regular "be:types" -- one of which might be a B_FILE_MIME_TYPE telling us it can also drop files
        for(int i=0; i< numTypesFound; i++) {
            auto _type = dropMsg->GetString(K_FIELD_TYPES, i, "x");

            if (!strcmp(_type, B_FILE_MIME_TYPE)) {
                hasFileTypes = true;
                if (i != numTypesFound - 1) {
                    printf("note: B_FILE_MIME_TYPE should only appear last in the be:types list\n");
                }
            } else {
                // non-file type
                hasDataTypes = true;
                auto msg = new BMessage(K_TYPES_MENU_SELECT);
                msg->SetString(K_FIELD_DEFAULT, _type);
                typesMenu->AddItem(new BMenuItem(_type, msg));
            }
        }

        // did the be:types have anything besides a file
        if (!hasDataTypes) {
            // well, apparently it can only drop files, so disable the whole data drop tab I guess
            tabView->RemoveTab(0);
        }

        // populate the file tab, if there is one
        if (hasFileTypes) {
            type_code fileTypesTypeCode;
            int32 numFileTypesFound;
            bool fileTypesFixedSize;
            if (dropMsg->GetInfo(K_FIELD_FILETYPES, &fileTypesTypeCode, &numFileTypesFound, &fileTypesFixedSize) == B_OK
                    && numFileTypesFound > 0 && fileTypesTypeCode == B_STRING_TYPE)
            {
                for(int i=0; i< numFileTypesFound; i++) {
                    auto fileType = dropMsg->GetString(K_FIELD_FILETYPES, i, "x");

                    auto msg = new BMessage(K_FILETYPES_MENU_SELECT);
                    msg->SetString(K_FIELD_DEFAULT, fileType);

                    fileTypesMenu->AddItem(new BMenuItem(fileType, msg));
                }
            } else {
                printf("drop message doesn't contain [%s] field of type B_STRING_TYPE", K_FIELD_FILETYPES);
                PostMessage(B_QUIT_REQUESTED);
                return;
            }
        } else {
            // remove file
            tabView->RemoveTab(1);
        }
    } else {
        printf("drop message doesn't contain [%s] field of type B_STRING_TYPE", K_FIELD_TYPES);
        PostMessage(B_QUIT_REQUESTED);
        return;
    }

    // enable buttons that are present in the actions list

    // populate the drop action menu
    type_code actionsTypeCode;
    int32 numActionsFound;
    bool actionsFixedSize;
    if (dropMsg->GetInfo(K_FIELD_ACTIONS, &actionsTypeCode, &numActionsFound, &actionsFixedSize) == B_OK
            && numActionsFound > 0 && actionsTypeCode == B_INT32_TYPE)
    {
        for(int i=0; i< numActionsFound; i++) {
            auto action = dropMsg->GetInt32(K_FIELD_ACTIONS, i, 0);

            switch(action) {
            case B_COPY_TARGET:
                dataActionButtons.copy->SetEnabled(true);
                fileActionButtons.copy->SetEnabled(true);
                break;
            case B_MOVE_TARGET:
                dataActionButtons.move->SetEnabled(true);
                fileActionButtons.move->SetEnabled(true);
                break;
            case B_LINK_TARGET:
                dataActionButtons.link->SetEnabled(true);
                fileActionButtons.link->SetEnabled(true);
                break;
            case B_TRASH_TARGET:
                dataActionButtons.trash->SetEnabled(true);
                fileActionButtons.trash->SetEnabled(true);
                break;
            }
        }
    } else {
        printf("drop message doesn't contain [%s] field of type B_INT32_TYPE", K_FIELD_ACTIONS);
        PostMessage(B_QUIT_REQUESTED);
        return;
    }
}


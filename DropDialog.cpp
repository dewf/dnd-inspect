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

BButton *addActionButton(const char *label, int32 action)
{
    auto msg = new BMessage(K_CLICKED_ACTION);
    msg->SetInt32(K_FIELD_DEFAULT, action);
    auto button = new BButton(label, msg);
    button->SetEnabled(false);
    return button;
}

DropDialog::DropDialog(BWindow *centerOn, BMessage *dropMsg)
    :BWindow(BRect(0, 0, 400, 200), "Drop parameters", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, 0),
      dropMsg(dropMsg)
{
	CenterIn(centerOn->Frame());

    typesMenu = new BPopUpMenu("(choose drop data type)");
    typeChooser = new BMenuField("Data type:", typesMenu);

    //dropAsFile = new BCheckBox("Drop as file", new BMessage(K_DROP_AS_FILE_CHECK));

    fileTypesMenu = new BPopUpMenu("(choose file data type)");
    fileTypeChooser = new BMenuField("File type:", fileTypesMenu);

    // choose file button
    chooseFileButton = new BButton("Pick" B_UTF8_ELLIPSIS, new BMessage(K_OPEN_FILE_CHOOSER));
    chosenPathLabel = new BStringView("path-label", "(no file selected)");

    actionMenu = new BPopUpMenu("(choose drop action)");
    actionChooser = new BMenuField("Drop action", actionMenu);

    // cancel / go buttons
    auto buttonsLayout = new BGroupLayout(B_HORIZONTAL);
    auto cancelButton = new BButton("Cancel", new BMessage(B_QUIT_REQUESTED));
    auto dropButton = new BButton("Drop", new BMessage(K_OK_PRESSED));

    // layout ========================================

    // tab area for data/file drop choice
    tabView = new BTabView("tabview");

    // data drop tab
	dataActionButtons.copy = addActionButton("Copy", B_COPY_TARGET);
    dataActionButtons.move = addActionButton("Move", B_MOVE_TARGET);
    dataActionButtons.link = addActionButton("Link", B_LINK_TARGET);
    dataActionButtons.trash = addActionButton("Trash", B_TRASH_TARGET);

	auto dataDropView = new BView("data", B_SUPPORTS_LAYOUT);
	dataDropView = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(typeChooser)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(B_USE_BIG_SPACING))
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(dataActionButtons.copy)
			.Add(dataActionButtons.move)
			.Add(dataActionButtons.link)
			.Add(dataActionButtons.trash)
			.AddGlue()
			.End()
		.AddGlue()
		.View();

    dataTab = new BTab();
    tabView->AddTab(dataDropView, dataTab);
    dataTab->SetLabel("Data drop");

    // file drop tab
    fileActionButtons.copy = addActionButton("Copy", B_COPY_TARGET);
    fileActionButtons.move = addActionButton("Move", B_MOVE_TARGET);
    fileActionButtons.link = addActionButton("Link", B_LINK_TARGET);
    fileActionButtons.trash = addActionButton("Trash", B_TRASH_TARGET);

	auto fileDropView = new BView("file", B_SUPPORTS_LAYOUT);
	fileDropView = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fileTypeChooser)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(chosenPathLabel)
			.Add(chooseFileButton)
			.End()
		.Add(BSpaceLayoutItem::CreateVerticalStrut(B_USE_BIG_SPACING))
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fileActionButtons.copy)
			.Add(fileActionButtons.move)
			.Add(fileActionButtons.link)
			.Add(fileActionButtons.trash)
			.AddGlue()
			.End()
		.AddGlue()
		.View();

    fileTab = new BTab();
    tabView->AddTab(fileDropView, fileTab);
    fileTab->SetLabel("File drop");

    //
	BLayoutBuilder::Group<>(this, B_VERTICAL, 1)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(B_USE_HALF_ITEM_SPACING))
		.AddGroup(B_VERTICAL)
			.SetInsets(-2)	// to remove the spacing around the tab view
			.Add(tabView)
			.End();

    // now actually fill and config those controls based on what was in the message
    configure();
}

bool DropDialog::GenerateResponse(BMessage **negotiationMsg)
{
    waitingThread = find_thread(nullptr);
    Show();

    thread_id sender;
    char buffer[64]; // not used, but currently necessary: https://review.haiku-os.org/c/haiku/+/1333
    while(receive_data(&sender, buffer, 64) != K_THREAD_NOTIFY) {
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
            // has the fild been chosen yet?
            if (!fileBeenChosen) {
                auto alert = new BAlert("Error", "You must select a destination file.", "OK");
                alert->SetShortcut(0, B_ESCAPE);
                alert->Go();
                return; // don't close dialog yet
            } else {
                negotiationMsg->AddString(K_FIELD_TYPES, B_FILE_MIME_TYPE);
                negotiationMsg->AddString(K_FIELD_FILETYPES, selectedFileType.c_str());
                negotiationMsg->AddRef("directory", &chosenDir);
                negotiationMsg->AddString("name", chosenFilename.c_str());
            }
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
		filePanel->Window()->SetTitle("Pick a file");
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
            fileBeenChosen = true;

            // touch the file first (because that's how haiku's tracker works / what the 'dragme' app expected)
            BDirectory dir(&chosenDir);
            BFile file(&dir, chosenFilename.c_str(), B_CREATE_FILE | B_WRITE_ONLY);
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
    char buffer[64]; // not used, but currently necessary: https://review.haiku-os.org/c/haiku/+/1333
    send_data(waitingThread, K_THREAD_NOTIFY, buffer, 64);

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


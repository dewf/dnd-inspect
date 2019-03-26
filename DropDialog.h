#pragma once

#include <InterfaceKit.h>

#include <string>

class BMenuField;
class BCheckBox;
class BButton;
class BFilePanel;
class BPopUpMenu;
class BStringView;

class DropDialog : public BWindow
{
    BMessage *dropMsg = nullptr;
    BMessage *negotiationMsg = nullptr; // created / configured by the dialog
    int32 dropAction = B_COPY_TARGET;

    BPopUpMenu *typesMenu = nullptr;
    BMenuField *typeChooser = nullptr;
    std::string selectedType; // can't use char* because the BMessage we're getting from has a finite lifespan

    BPopUpMenu *fileTypesMenu = nullptr;
    BMenuField *fileTypeChooser = nullptr;
    std::string selectedFileType; // see note on selectedType above

    BButton *chooseFileButton = nullptr;
    BFilePanel *filePanel = nullptr;
    BStringView *selectedPathLabel = nullptr;

    BPopUpMenu *actionMenu = nullptr;
    BMenuField *actionChooser = nullptr;

    thread_id waitingThread;

    void MessageReceived(BMessage *msg) override;
    bool QuitRequested() override;

    void configure(); // configure all the empty controls with relevant data / state
public:
    DropDialog(BWindow *centerOn, BMessage *dropMsg);

    bool GenerateResponse(BMessage **negotiationMsg); // modal show
};

#pragma once

#include <InterfaceKit.h>

#include <string>

class BMenuField;
class BCheckBox;
class BButton;
class BFilePanel;

class DropDialog : public BWindow
{
    BMessage *dropMsg = nullptr;
    BMessage *negotiationMsg = nullptr; // created / configured by the dialog
    int32 dropAction = B_COPY_TARGET;

    BMenuField *typeChooser = nullptr;
    std::string selectedType; // can't use char* because the BMessage we're getting from has a finite lifespan

    BCheckBox *dropAsFile = nullptr;
    BButton *chooseFileButton = nullptr;
    BFilePanel *filePanel = nullptr;

    BMenuField *fileTypeChooser = nullptr;
    std::string selectedFileType;

    thread_id waitingThread;

    void MessageReceived(BMessage *msg) override;
    bool QuitRequested() override;
public:
    DropDialog(BWindow *centerOn, BMessage *dropMsg);

    bool GenerateResponse(BMessage **negotiationMsg);
};

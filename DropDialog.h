#pragma once

#include <InterfaceKit.h>

class BMenuField;

class DropDialog : public BWindow
{
    BMessage *dropMsg;
    int32 dropAction = B_COPY_TARGET;

    BMenuField *typeChooser = nullptr;
    const char *selectedType = nullptr;

    BMenuField *fileTypeChooser = nullptr;
    const char *selectedFileType = nullptr;
public:
    DropDialog(BWindow *centerOn, BMessage *dropMsg);

    void MessageReceived(BMessage *msg) override;
};

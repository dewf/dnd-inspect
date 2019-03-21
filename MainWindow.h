#pragma once

#include <Window.h>
#include "DragSourceView.h"
#include "DropTargetView.h"

class MainWindow : public BWindow
{
    DragSourceView *dragSourceView = nullptr;
    DropTargetView *dropTargetView = nullptr;

    void MessageReceived(BMessage *message) override;
public:
    MainWindow();
    ~MainWindow() override;
};


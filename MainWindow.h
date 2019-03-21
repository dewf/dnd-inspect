#pragma once

#include <Window.h>
#include "DragSourceView.h"
#include "DropTargetView.h"

class MainWindow : public BWindow
{
    DragSourceView *dragSourceView = nullptr;
    DropTargetView *dropTargetView = nullptr;
public:
    MainWindow();
};


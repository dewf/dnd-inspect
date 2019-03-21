#include "MainWindow.h"

#include <TabView.h>

BRect getCenterRect(int width, int height)
{
    auto x = 100;
    auto y = 100;
    return BRect(x, y, x + width, y + height);
}

MainWindow::MainWindow()
    :BWindow(getCenterRect(500, 600), "DND Inspector", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
{
    auto tabView = new BTabView(Bounds(), "tabview");

    auto r = tabView->Bounds();
    r.bottom -= tabView->TabHeight(); // essentially adjusting the height, not bottom per se

    auto tab = new BTab();
    auto dragSource = new DragSourceView(r);
    tabView->AddTab(dragSource, tab);
    tab->SetLabel("Drag Source");

    auto dropTarget = new DropTargetView(r);
    auto tab2 = new BTab();
    tabView->AddTab(dropTarget, tab2);
    tab2->SetLabel("Drop Target");

    AddChild(tabView);
}

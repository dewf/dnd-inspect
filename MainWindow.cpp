#include "MainWindow.h"

#include <AppKit.h>
#include <InterfaceKit.h>
#include <TabView.h>

#include <stdio.h>

static const int32 K_NEW_WINDOW_MSG = 'NWMg';

static int numWindowsOpen = 0;

BRect getCenterRect(int width, int height)
{
    BScreen screen;
    auto frame = screen.Frame();
    auto x = (frame.Width() - width) / 2;
    auto y = (frame.Height() - height) / 2;
    return BRect(x, y, x + width, y + height);
}

void MainWindow::MessageReceived(BMessage *message)
{
    switch(message->what) {
    case K_NEW_WINDOW_MSG:
    {
        BPoint p(50, 50);
        ConvertToScreen(&p);

        auto win = new MainWindow();
        win->MoveTo(p);

        win->Show();
        break;
    }
    default:
        BWindow::MessageReceived(message);
    }
}

MainWindow::MainWindow()
    :BWindow(getCenterRect(500, 600), "DND Inspector", B_TITLED_WINDOW, 0 /*B_QUIT_ON_WINDOW_CLOSE*/)
{
    // menu
    auto menuBar = new BMenuBar(Bounds(), "menubar");
    auto fileMenu = new BMenu("File");
    fileMenu->AddItem(new BMenuItem("New Window", new BMessage(K_NEW_WINDOW_MSG)));
    menuBar->AddItem(new BMenuItem(fileMenu));
    AddChild(menuBar);

    auto menuHeight = menuBar->PreferredSize().Height() + 1;

    auto bounds = Bounds();
    bounds.top += menuHeight;

    // content
    auto tabView = new BTabView(bounds, "tabview");

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

    numWindowsOpen++;
}

MainWindow::~MainWindow()
{
    numWindowsOpen--;
    if (numWindowsOpen < 1) {
        be_app->PostMessage(B_QUIT_REQUESTED);
    }
}

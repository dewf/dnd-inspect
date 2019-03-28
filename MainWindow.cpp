#include "MainWindow.h"

#include <AppKit.h>
#include <InterfaceKit.h>
#include <LayoutBuilder.h>
#include <TabView.h>

#include <stdio.h>

#include "Globals.h"
#include "DNDEncoder.h"

static const int32 K_NEW_WINDOW_MSG = 'NWMg';
static const int32 K_QUIT_APP_MSG = 'EXiT';

static int numWindowsOpen = 0;

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
    case K_QUIT_APP_MSG:
    {
        be_app->PostMessage(B_QUIT_REQUESTED);
        break;
    }
    default:
        BWindow::MessageReceived(message);
    }
}

MainWindow::MainWindow()
    :BWindow(BRect(0, 0, 300, 400), "DND Inspector", B_TITLED_WINDOW, 0 /*B_QUIT_ON_WINDOW_CLOSE*/)
{
    CenterOnScreen();

    // menu
    auto menuBar = new BMenuBar("menubar");
    auto fileMenu = new BMenu("File");
    fileMenu->AddItem(new BMenuItem("New window", new BMessage(K_NEW_WINDOW_MSG), 'N'));
    fileMenu->AddItem(new BMenuItem("Quit", new BMessage(K_QUIT_APP_MSG), 'Q'));
    menuBar->AddItem(new BMenuItem(fileMenu));

    // content
    auto tabView = new BTabView("tabview");

    dragSourceView = new DragSourceView();
    auto tab = new BTab();
    tabView->AddTab(dragSourceView, tab);
    tab->SetLabel("Drag source");

    dropTargetView = new DropTargetView();
    auto tab2 = new BTab();
    tabView->AddTab(dropTargetView, tab2);
    tab2->SetLabel("Drop target");

	tabView->SetBorder(B_NO_BORDER);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 1)
		.Add(menuBar)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(B_USE_HALF_ITEM_SPACING))
		.AddGroup(B_VERTICAL)
			.SetInsets(-2)	// to remove the spacing around the tab view
			.Add(tabView);

	SetSizeLimits(250, B_SIZE_UNLIMITED, 100, B_SIZE_UNLIMITED);

    numWindowsOpen++;
}

MainWindow::~MainWindow()
{
    numWindowsOpen--;
    if (numWindowsOpen < 1) {
        be_app->PostMessage(B_QUIT_REQUESTED);
    }
}

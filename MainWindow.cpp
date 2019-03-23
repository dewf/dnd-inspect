#include "MainWindow.h"

#include <AppKit.h>
#include <InterfaceKit.h>
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
    case B_COPY_TARGET:
    case B_MOVE_TARGET:
    case B_LINK_TARGET:
    case B_TRASH_TARGET:
    {
        // dnd negotiation reply - forward to the encoder that created it
        if (message->IsReply()) {
            auto prev = message->Previous();
            auto encoder = (DNDEncoder *)prev->GetPointer(K_FIELD_ORIGINATOR);
            if (encoder) {
                encoder->finalizeDrop(message);
            } else {
                printf("no originator/encoder field in negotiation message?\n");
            }
        }
        break;
    }
    case B_MIME_DATA:
    {
        // got here from App::MessageReceived
        dropTargetView->processFinalDrop(message);
        break;
    }
    default:
        BWindow::MessageReceived(message);
    }
}

BRect getCenterRect(int width, int height)
{
    BScreen screen;
    auto frame = screen.Frame();
    auto x = (frame.Width() - width) / 2;
    auto y = (frame.Height() - height) / 2;
    return BRect(x, y, x + width, y + height);
}

MainWindow::MainWindow()
    :BWindow(getCenterRect(500, 600), "DND Inspector", B_TITLED_WINDOW, 0 /*B_QUIT_ON_WINDOW_CLOSE*/)
{
    // menu
    auto menuBar = new BMenuBar(Bounds(), "menubar");
    auto fileMenu = new BMenu("File");
    fileMenu->AddItem(new BMenuItem("New Window", new BMessage(K_NEW_WINDOW_MSG)));
    fileMenu->AddItem(new BMenuItem("Quit", new BMessage(K_QUIT_APP_MSG), 'Q', B_COMMAND_KEY));
    menuBar->AddItem(new BMenuItem(fileMenu));
    AddChild(menuBar);

    auto menuHeight = menuBar->PreferredSize().Height() + 1;

    auto bounds = Bounds();
    bounds.top += menuHeight;

    // content
    auto tabView = new BTabView(bounds, "tabview");

    auto r = tabView->Bounds();
    r.bottom -= tabView->TabHeight(); // essentially adjusting the height, not bottom per se

    dragSourceView = new DragSourceView(r);
    auto tab = new BTab();
    tabView->AddTab(dragSourceView, tab);
    tab->SetLabel("Drag Source");

    dropTargetView = new DropTargetView(r);
    auto tab2 = new BTab();
    tabView->AddTab(dropTargetView, tab2);
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

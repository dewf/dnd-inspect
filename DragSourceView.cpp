#include "DragSourceView.h"

#include <MenuBar.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <ScrollView.h>

#include <stdio.h>

#include "DragSourceList.h"

DragSourceView::DragSourceView()
    :BView("drag_source", B_WILL_DRAW , new BGroupLayout(B_VERTICAL, B_USE_DEFAULT_SPACING))
{
    AdoptSystemColors();

    auto list = new DragSourceList();

//    for(int i=0; i< 10; i++) {
//        char buffer[1024];
//        sprintf(buffer, "Format %02d\n", i);
//        list->AddItem(new BStringItem(buffer));
//    }

    auto scroll = new BScrollView("scrollview", list, B_WILL_DRAW, false, true);
    AddChild(scroll);
}

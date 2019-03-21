#include "DragSourceView.h"

#include <MenuBar.h>

#include <ListView.h>
#include <ScrollView.h>

#include <stdio.h>

#include "DragSourceList.h"

DragSourceView::DragSourceView(BRect r)
    :BView(r, "drag_source", B_FOLLOW_ALL, 0)
{
    AdoptSystemColors();

    r = Bounds();
    r.InsetBy(10, 10);
    r.right -= 22;
    r.bottom -= 10;
    // no flippin' idea, none of these bounds make sense :(
    auto list = new DragSourceList(r);

//    for(int i=0; i< 10; i++) {
//        char buffer[1024];
//        sprintf(buffer, "Format %02d\n", i);
//        list->AddItem(new BStringItem(buffer));
//    }

    auto scroll = new BScrollView("scrollview", list, B_FOLLOW_ALL, 0, false, true);
    AddChild(scroll);
}

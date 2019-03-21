#include "DropTargetView.h"

DropTargetView::DropTargetView(BRect r)
    :BView(r, "droptarget", B_FOLLOW_ALL, 0)
{
    AdoptSystemColors();
}

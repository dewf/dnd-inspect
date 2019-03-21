#include "MyTabView.h"

#include <stdio.h>

MyTabView::MyTabView()
    :BTabView("tabview1")
{
    // nothing yet
}

MyTabView::~MyTabView()
{
    printf("MyTabView destroyed\n");
}

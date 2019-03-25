#pragma once

#include <View.h>

class DroppableTextView;

class DropTargetView : public BView
{
    DroppableTextView *logArea = nullptr;
public:
    DropTargetView(BRect r);
};

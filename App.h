#pragma once

#include <Application.h>

class App : public BApplication
{
    void MessageReceived(BMessage *msg) override;
public:
    App();
};

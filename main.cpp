#include <stdio.h>
#include "App.h"
#include "MainWindow.h"

int main(int argc, char **argv)
{
    App app;

    auto win = new MainWindow();
    win->Show();

    app.Run();
    return 0;
}

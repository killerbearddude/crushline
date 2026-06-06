#include <SDL3/SDL_main.h>

#include "app/App.h"

#include <iostream>

int main(int, char**)
{
    App app;

    if (!app.Initialize())
    {
        std::cerr << "Failed to initialize app.\n";
        return 1;
    }

    while (!app.ShouldClose())
    {
        app.RunFrame();
    }

    app.Shutdown();
    return 0;
}

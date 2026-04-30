#include "application.h"

#include <iostream>

int main() {
    Application app;
    if (!app.init(1280, 720, "TrueShot - Tactical FPS")) {
        std::cerr << "Failed to initialize TrueShot.\n";
        return 1;
    }
    return app.run();
}

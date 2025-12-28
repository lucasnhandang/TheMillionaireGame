#include "gui_app.h"
#include <iostream>

int main(int argc, char* argv[]) {
    GuiApp app;
    
    if (!app.initialize(1280, 720)) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }
    
    app.run();
    
    return 0;
}


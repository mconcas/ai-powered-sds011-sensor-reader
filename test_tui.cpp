#include "interactive_tui.h"
#include <iostream>

int main() {
    std::cout << "Starting minimal TUI test..." << std::endl;
    
    InteractiveTUI tui;
    if (!tui.initialize()) {
        std::cerr << "Failed to initialize TUI" << std::endl;
        return 1;
    }
    
    // Just show the sensor menu once and then exit
    tui.showSensorMenu();
    
    // Wait for a key press
    getch();
    
    // Clean up
    endwin();
    
    std::cout << "TUI test completed" << std::endl;
    return 0;
}

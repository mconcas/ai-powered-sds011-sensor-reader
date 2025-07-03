#include "../include/interactive_tui.h"
#include <iostream>
#include <signal.h>

/**
 * @brief Simple TUI test program
 * 
 * This program tests the interactive TUI functionality.
 */

// Global flag for clean shutdown
volatile bool g_running = true;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_running = false;
    }
}

int main() {
    std::cout << "Testing Interactive TUI..." << std::endl;
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    InteractiveTUI tui;
    if (!tui.initialize()) {
        std::cerr << "Failed to initialize TUI" << std::endl;
        return 1;
    }
    
    std::cout << "TUI initialized successfully. Starting main loop..." << std::endl;
    tui.run();
    
    std::cout << "TUI test completed." << std::endl;
    return 0;
}

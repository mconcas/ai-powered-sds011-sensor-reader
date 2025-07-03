#pragma once

#include <string>

/**
 * @brief Application utilities and helper functions
 */
namespace AppUtils {
    /**
     * @brief Print usage information
     * @param program_name The name of the program executable
     */
    void printUsage(const char* program_name);
    
    /**
     * @brief Signal handler for graceful shutdown
     * @param signal The signal number received
     */
    void signalHandler(int signal);
    
    /**
     * @brief Parse command line arguments
     * @param argc Number of command line arguments
     * @param argv Array of command line arguments
     * @param serial_port Reference to store the serial port
     * @param use_tui Reference to store TUI preference
     * @return true if arguments were parsed successfully, false if help was requested
     */
    bool parseArguments(int argc, char* argv[], std::string& serial_port, bool& use_tui);
}

// Global flag for clean shutdown
extern volatile bool g_running;

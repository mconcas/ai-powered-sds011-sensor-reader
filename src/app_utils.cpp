#include "app_utils.h"
#include <iostream>
#include <signal.h>

// Global flag for clean shutdown
volatile bool g_running = true;

namespace AppUtils {
    void signalHandler(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            g_running = false;
        }
    }
    
    void printUsage(const char* program_name) {
        std::cout << "Usage: " << program_name << " [options] [serial_port]" << std::endl;
        std::cout << "  Options:" << std::endl;
        std::cout << "    --no-tui    Disable TUI mode and use console output" << std::endl;
        std::cout << "    -h, --help  Show this help message" << std::endl;
        std::cout << "  serial_port: Serial port device (default: /dev/ttyUSB0)" << std::endl;
        std::cout << std::endl;
        std::cout << "  TUI Controls:" << std::endl;
        std::cout << "    q    Quit the program" << std::endl;
        std::cout << "    c    Clear all collected data" << std::endl;
        std::cout << std::endl;
        std::cout << "  Examples:" << std::endl;
        std::cout << "    " << program_name << "                    # TUI mode with default port" << std::endl;
        std::cout << "    " << program_name << " /dev/ttyUSB1       # TUI mode with custom port" << std::endl;
        std::cout << "    " << program_name << " --no-tui           # Console mode with default port" << std::endl;
        std::cout << "    " << program_name << " --no-tui /dev/ttyACM0  # Console mode with custom port" << std::endl;
    }
    
    bool parseArguments(int argc, char* argv[], std::string& serial_port, bool& use_tui) {
        serial_port = "/dev/ttyUSB0";
        use_tui = true;
        
        if (argc > 1) {
            if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
                printUsage(argv[0]);
                return false;
            }
            if (std::string(argv[1]) == "--no-tui") {
                use_tui = false;
                if (argc > 2) {
                    serial_port = argv[2];
                }
            } else {
                serial_port = argv[1];
            }
        }
        
        return true;
    }
}

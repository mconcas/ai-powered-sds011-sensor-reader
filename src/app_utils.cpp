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
        std::cout << "    --legacy    Use legacy single-sensor mode instead of interactive" << std::endl;
        std::cout << "    -h, --help  Show this help message" << std::endl;
        std::cout << "  serial_port: Serial port device (default: /dev/ttyUSB0)" << std::endl;
        std::cout << std::endl;
        std::cout << "  Interactive Mode Controls:" << std::endl;
        std::cout << "    ↑↓         Navigate sensor list" << std::endl;
        std::cout << "    Enter      Connect to selected sensor" << std::endl;
        std::cout << "    r          Refresh sensor list" << std::endl;
        std::cout << "    b          Back to sensor selection" << std::endl;
        std::cout << "    c          Clear collected data" << std::endl;
        std::cout << "    q          Quit the program" << std::endl;
        std::cout << std::endl;
        std::cout << "  Examples:" << std::endl;
        std::cout << "    " << program_name << "                    # Interactive mode (default)" << std::endl;
        std::cout << "    " << program_name << " --legacy           # Legacy TUI mode with default port" << std::endl;
        std::cout << "    " << program_name << " --legacy /dev/ttyUSB1  # Legacy TUI mode with custom port" << std::endl;
        std::cout << "    " << program_name << " --no-tui           # Console mode with default port" << std::endl;
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

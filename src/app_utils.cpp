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
#ifdef MACOS
        std::cout << "  serial_port: Serial port device (default: /dev/cu.usbserial)" << std::endl;
#else
        std::cout << "  serial_port: Serial port device (default: /dev/ttyUSB0)" << std::endl;
#endif
        std::cout << std::endl;
        std::cout << "  Interactive Mode Controls:" << std::endl;
        std::cout << "    ^v         Navigate sensor list (up/down arrows)" << std::endl;
        std::cout << "    Enter      Connect to selected sensor" << std::endl;
        std::cout << "    r          Refresh sensor list" << std::endl;
        std::cout << "    b          Back to sensor selection" << std::endl;
        std::cout << "    c          Clear collected data" << std::endl;
        std::cout << "    q          Quit the program" << std::endl;
        std::cout << std::endl;
        std::cout << "  Examples:" << std::endl;
        std::cout << "    " << program_name << "                    # Interactive mode (default)" << std::endl;
        std::cout << "    " << program_name << " --legacy           # Legacy TUI mode with default port" << std::endl;
#ifdef MACOS
        std::cout << "    " << program_name << " --legacy /dev/cu.usbserial-1  # Legacy TUI mode with custom port" << std::endl;
#else
        std::cout << "    " << program_name << " --legacy /dev/ttyUSB1  # Legacy TUI mode with custom port" << std::endl;
#endif
        std::cout << "    " << program_name << " --no-tui           # Console mode with default port" << std::endl;
    }
    
    bool parseArguments(int argc, char* argv[], std::string& serial_port, bool& use_tui) {
#ifdef MACOS
        serial_port = "/dev/cu.usbserial";
#else
        serial_port = "/dev/ttyUSB0";
#endif
        use_tui = true;
        
        bool found_port = false;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "-h" || arg == "--help") {
                printUsage(argv[0]);
                return false;
            } else if (arg == "--no-tui") {
                use_tui = false;
            } else if (arg == "--legacy") {
                // Legacy flag handled in main()
                continue;
            } else if (!found_port && arg[0] != '-') {
                // This is the serial port argument
                serial_port = arg;
                found_port = true;
            }
        }
        
        return true;
    }
}

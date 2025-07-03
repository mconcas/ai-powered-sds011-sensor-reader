#include "sds011_reader.h"
#include "interactive_tui.h"
#include "app_utils.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <signal.h>

/**
 * @brief Console mode implementation
 * @param sensor The SDS011 sensor reader instance
 * @param serial_port The serial port being used
 */
void runConsoleMode(SDS011Reader& sensor, const std::string& serial_port) {
    std::cout << "SDS011 PM2.5 Sensor Reader - Console Mode" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Product model: SDS011 V1.3" << std::endl;
    std::cout << "Serial port: " << serial_port << std::endl;
    std::cout << "Use --no-tui to disable TUI mode" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Reading PM2.5 data (Press Ctrl+C to exit)..." << std::endl;
    std::cout << std::endl;
    
    // Print header
    std::cout << std::setw(20) << "Timestamp"
              << std::setw(12) << "PM2.5 (µg/m³)"
              << std::setw(12) << "PM10 (µg/m³)"
              << std::endl;
    std::cout << std::string(44, '-') << std::endl;
    
    // Main reading loop
    int reading_count = 0;
    while (g_running) {
        float pm25, pm10;
        
        if (sensor.readPM25Data(pm25, pm10)) {
            // Get current timestamp
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);
            
            // Print formatted data
            std::cout << std::setw(2) << std::setfill('0') << tm.tm_hour << ":"
                     << std::setw(2) << std::setfill('0') << tm.tm_min << ":"
                     << std::setw(2) << std::setfill('0') << tm.tm_sec
                     << std::setw(12) << std::setfill(' ') << std::fixed << std::setprecision(1) << pm25
                     << std::setw(12) << std::setprecision(1) << pm10
                     << std::endl;
            
            reading_count++;
            
            // Print summary every 10 readings
            if (reading_count % 10 == 0) {
                std::cout << std::endl << "Readings collected: " << reading_count << std::endl;
                std::cout << std::string(44, '-') << std::endl;
            }
        } else {
            std::cerr << "Failed to read valid data from sensor" << std::endl;
        }
        
        // Wait before next reading (SDS011 updates every ~1 second)
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main(int argc, char* argv[]) {
    std::string serial_port;
    bool use_tui;
    
    // Parse command line arguments
    if (!AppUtils::parseArguments(argc, argv, serial_port, use_tui)) {
        return 0; // Help was displayed
    }
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, AppUtils::signalHandler);
    signal(SIGTERM, AppUtils::signalHandler);
    
    if (use_tui) {
        // Interactive mode with modern TUI
        std::cout << "Initializing interactive TUI..." << std::endl;
        InteractiveTUI interactive;
        if (!interactive.initialize()) {
            std::cerr << "Failed to initialize interactive TUI. Falling back to console mode." << std::endl;
            use_tui = false;
        } else {
            std::cout << "Starting interactive sensor monitor..." << std::endl;
            interactive.run();
            return 0;
        }
    }
    
    // Console mode - direct sensor operation
    std::cout << "Starting in console mode..." << std::endl;
    
    // Initialize sensor reader
    SDS011Reader sensor(serial_port);
    if (!sensor.initialize()) {
        std::cerr << "Failed to initialize sensor. Please check:" << std::endl;
        std::cerr << "  - Serial port exists and is accessible" << std::endl;
        std::cerr << "  - User has permission to access the port" << std::endl;
        std::cerr << "  - SDS011 sensor is connected and powered on" << std::endl;
        return 1;
    }
    
    runConsoleMode(sensor, serial_port);
    return 0;
}

#include "plugin_manager.h"
#include "interactive_tui.h"
#include "app_utils.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <signal.h>

/**
 * @brief Console mode implementation using plugin system
 * @param device The device information
 * @param plugin The sensor plugin to use
 */
void runConsoleMode(const DeviceInfo& device, Plugin* plugin) {
    std::cout << "Dynamic Sensor Reader - Console Mode" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "Plugin: " << plugin->getPluginName() << " v" << plugin->getVersion() << std::endl;
    std::cout << "Device: " << device.description << std::endl;
    std::cout << "Serial port: " << device.port << std::endl;
    std::cout << "Use --no-tui to disable TUI mode" << std::endl;
    std::cout << std::endl;
    
    // Create sensor instance
    auto sensor = plugin->createSensor();
    if (!sensor) {
        std::cerr << "Failed to create sensor instance" << std::endl;
        return;
    }
    
    if (!sensor->initialize(device.port)) {
        std::cerr << "Failed to initialize sensor" << std::endl;
        return;
    }
    
    std::cout << "Reading sensor data (Press Ctrl+C to exit)..." << std::endl;
    std::cout << std::endl;
    
    // Print header
    std::cout << std::setw(20) << "Timestamp"
              << std::setw(50) << "Sensor Data"
              << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    // Main reading loop
    int reading_count = 0;
    while (g_running) {
        auto data = sensor->readData();
        
        if (data) {
            // Get current timestamp
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);
            
            // Print formatted data
            std::cout << std::setw(2) << std::setfill('0') << tm.tm_hour << ":"
                     << std::setw(2) << std::setfill('0') << tm.tm_min << ":"
                     << std::setw(2) << std::setfill('0') << tm.tm_sec
                     << std::setw(50) << std::setfill(' ') << data->getDisplayString()
                     << std::endl;
            
            reading_count++;
            
            // Print summary every 10 readings
            if (reading_count % 10 == 0) {
                std::cout << std::endl << "Readings collected: " << reading_count << std::endl;
                std::cout << std::string(70, '-') << std::endl;
            }
        } else {
            std::cerr << "Failed to read valid data from sensor" << std::endl;
        }
        
        // Wait before next reading
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    sensor->cleanup();
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
        // Interactive mode with plugin-based TUI
        std::cout << "Initializing interactive TUI with plugin system..." << std::endl;
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
    
    // Console mode - use plugin system for device detection and selection
    std::cout << "Starting in console mode with plugin system..." << std::endl;
    
    // Initialize plugin manager
    PluginManager pluginManager("./build/plugins");
    if (!pluginManager.loadAllPlugins()) {
        std::cerr << "Failed to load plugins" << std::endl;
        return 1;
    }
    
    // Discover devices
    auto devices = pluginManager.detectAllDevices();
    if (devices.empty()) {
        std::cerr << "No compatible devices found" << std::endl;
        return 1;
    }
    
    // Use the first accessible device
    DeviceInfo selectedDevice;
    Plugin* selectedPlugin = nullptr;
    
    for (const auto& device : devices) {
        if (device.accessible) {
            selectedPlugin = pluginManager.findBestPluginForDevice(device);
            if (selectedPlugin) {
                selectedDevice = device;
                break;
            }
        }
    }
    
    if (!selectedPlugin) {
        std::cerr << "No compatible plugins found for available devices" << std::endl;
        return 1;
    }
    
    std::cout << "Selected device: " << selectedDevice.description 
              << " at " << selectedDevice.port << std::endl;
    std::cout << "Using plugin: " << selectedPlugin->getPluginName() 
              << " v" << selectedPlugin->getVersion() << std::endl;
    
    runConsoleMode(selectedDevice, selectedPlugin);
    return 0;
}

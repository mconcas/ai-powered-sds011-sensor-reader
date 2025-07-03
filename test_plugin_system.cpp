#include "plugin_manager.h"
#include <iostream>

int main() {
    std::cout << "Testing Dynamic Plugin System..." << std::endl;
    
    // Create plugin manager and load plugins
    PluginManager pluginManager("./build/plugins");
    
    std::cout << "\nLoading plugins..." << std::endl;
    bool success = pluginManager.loadAllPlugins();
    if (!success) {
        std::cout << "Warning: Some plugins failed to load" << std::endl;
    }
    
    // List loaded plugins
    auto pluginNames = pluginManager.getPluginNames();
    std::cout << "\nLoaded " << pluginNames.size() << " plugin(s):" << std::endl;
    for (const auto& name : pluginNames) {
        Plugin* plugin = pluginManager.getPluginByName(name);
        if (plugin) {
            std::cout << "  - " << name << " v" << plugin->getVersion() 
                     << ": " << plugin->getDescription() << std::endl;
        }
    }
    
    // Discover devices
    std::cout << "\nDiscovering devices..." << std::endl;
    auto devices = pluginManager.detectAllDevices();
    
    if (devices.empty()) {
        std::cout << "No devices found." << std::endl;
    } else {
        std::cout << "Found " << devices.size() << " device(s):" << std::endl;
        std::cout << std::endl;
        
        printf("%-20s %-15s %-15s %-15s %-15s %s\n", 
               "Port", "Vendor ID", "Product ID", "Description", "Plugin", "Status");
        printf("%s\n", std::string(100, '-').c_str());
        
        for (const auto& device : devices) {
            Plugin* plugin = pluginManager.findBestPluginForDevice(device);
            std::string pluginName = plugin ? plugin->getPluginName() : "None";
            std::string status = device.accessible ? "Accessible" : "Blocked";
            
            printf("%-20s %-15s %-15s %-15s %-15s %s\n",
                   device.port.c_str(),
                   device.vendor_id.c_str(),
                   device.product_id.c_str(),
                   device.description.c_str(),
                   pluginName.c_str(),
                   status.c_str());
        }
        
        // Test plugin functionality for accessible devices
        std::cout << "\nTesting plugin functionality..." << std::endl;
        for (const auto& device : devices) {
            if (!device.accessible) continue;
            
            Plugin* plugin = pluginManager.findBestPluginForDevice(device);
            if (!plugin) {
                std::cout << "No plugin available for " << device.port << std::endl;
                continue;
            }
            
            std::cout << "Testing " << plugin->getPluginName() 
                     << " plugin with device " << device.port << "..." << std::endl;
            
            // Create sensor and UI components
            auto sensor = plugin->createSensor();
            auto ui = plugin->createUI();
            
            if (!sensor || !ui) {
                std::cout << "  Failed to create plugin components" << std::endl;
                continue;
            }
            
            std::cout << "  Plugin components created successfully" << std::endl;
            std::cout << "  Sensor: " << sensor->getSensorName() 
                     << " v" << sensor->getVersion() << std::endl;
            std::cout << "  UI: " << ui->getPluginName() 
                     << " v" << ui->getVersion() << std::endl;
            
            // Test initialization (without actually connecting)
            std::cout << "  Testing sensor initialization..." << std::endl;
            if (sensor->initialize(device.port)) {
                std::cout << "  ✓ Sensor initialized successfully" << std::endl;
                
                // Check if connected
                if (sensor->isConnected()) {
                    std::cout << "  ✓ Sensor connected and ready" << std::endl;
                } else {
                    std::cout << "  ! Sensor initialized but not connected" << std::endl;
                }
                
                sensor->cleanup();
            } else {
                std::cout << "  ✗ Sensor initialization failed" << std::endl;
            }
        }
    }
    
    std::cout << "\nPlugin system test completed." << std::endl;
    return 0;
}

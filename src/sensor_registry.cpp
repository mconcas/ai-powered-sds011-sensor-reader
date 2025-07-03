#include "sensor_registry.h"
#include "sds011_plugin.h"
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

void SensorRegistry::registerPlugin(std::unique_ptr<SensorPlugin> plugin) {
    if (plugin) {
        std::string type = plugin->getTypeName();
        plugins[type] = std::move(plugin);
    }
}

std::vector<std::string> SensorRegistry::getAvailableTypes() const {
    std::vector<std::string> types;
    for (const auto& pair : plugins) {
        types.push_back(pair.first);
    }
    return types;
}

std::unique_ptr<SensorPlugin> SensorRegistry::createPlugin(const std::string& type) const {
    auto it = plugins.find(type);
    if (it != plugins.end()) {
        // Create a new instance based on type
        if (type == "SDS011") {
            return std::unique_ptr<SensorPlugin>(new SDS011Plugin());
        }
    }
    return std::unique_ptr<SensorPlugin>();
}

std::vector<SensorInfo> SensorRegistry::discoverSensors() const {
    std::vector<SensorInfo> sensors;
    std::vector<std::string> ports = getCommonPorts();
    
    for (const auto& port : ports) {
        // Check if port exists
        struct stat st;
        if (stat(port.c_str(), &st) != 0) {
            continue;
        }
        
        // Test each plugin type
        for (const auto& pair : plugins) {
            const std::string& type = pair.first;
            auto plugin = createPlugin(type);
            
            if (plugin && plugin->isAvailable(port)) {
                sensors.emplace_back(port, type, plugin->getDescription(), true);
                break; // Port taken by this sensor type
            }
        }
        
        // If no sensor detected, still show port as available
        if (sensors.empty() || sensors.back().port != port) {
            sensors.emplace_back(port, "Unknown", "Unidentified device", false);
        }
    }
    
    return sensors;
}

std::vector<std::string> SensorRegistry::getCommonPorts() {
    std::vector<std::string> ports;
    
#ifdef MACOS
    // macOS USB serial ports
    for (int i = 0; i < 4; ++i) {
        ports.push_back("/dev/cu.usbserial-" + std::to_string(i));
        ports.push_back("/dev/cu.usbmodem" + std::to_string(i));
        ports.push_back("/dev/cu.SLAB_USBtoUART" + std::to_string(i));
    }
    
    // Common macOS serial ports
    ports.push_back("/dev/cu.usbserial");
    ports.push_back("/dev/cu.usbmodem");
    ports.push_back("/dev/cu.SLAB_USBtoUART");
    
#elif defined(LINUX)
    // Linux USB serial ports
    for (int i = 0; i < 4; ++i) {
        ports.push_back("/dev/ttyUSB" + std::to_string(i));
        ports.push_back("/dev/ttyACM" + std::to_string(i));
    }
    
    // Common embedded serial ports
    ports.push_back("/dev/ttyS0");
    ports.push_back("/dev/ttyS1");
    
#else
    // Generic fallback
    ports.push_back("/dev/ttyUSB0");
    ports.push_back("/dev/ttyACM0");
#endif
    
    return ports;
}

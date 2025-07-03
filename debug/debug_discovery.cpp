#include "sensor_registry.h"
#include "sds011_plugin.h"
#include <iostream>

int main() {
    std::cout << "Testing sensor discovery..." << std::endl;
    
    SensorRegistry registry;
    registry.registerPlugin(std::unique_ptr<SensorPlugin>(new SDS011Plugin()));
    
    std::cout << "Registered plugins: " << registry.getAvailableTypes().size() << std::endl;
    for (const auto& type : registry.getAvailableTypes()) {
        std::cout << "  - " << type << std::endl;
    }
    
    std::cout << "Testing /dev/ttyUSB0 directly..." << std::endl;
    SDS011Plugin test_plugin;
    bool available = test_plugin.isAvailable("/dev/ttyUSB0");
    std::cout << "SDS011Plugin::isAvailable(\"/dev/ttyUSB0\") = " << available << std::endl;
    
    std::cout << "Discovering sensors..." << std::endl;
    auto sensors = registry.discoverSensors();
    std::cout << "Found " << sensors.size() << " sensors:" << std::endl;
    
    for (const auto& sensor : sensors) {
        std::cout << "  Port: " << sensor.port 
                  << ", Type: " << sensor.type 
                  << ", Available: " << (sensor.available ? "Yes" : "No")
                  << ", Description: " << sensor.description << std::endl;
    }
    
    return 0;
}

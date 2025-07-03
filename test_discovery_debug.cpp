#include "sensor_registry.h"
#include "sds011_plugin.h"
#include <iostream>

int main() {
    std::cout << "Testing device discovery..." << std::endl;
    
    SensorRegistry registry;
    registry.registerPlugin(std::unique_ptr<SensorPlugin>(new SDS011Plugin()));
    
    std::cout << "Discovering sensors..." << std::endl;
    auto sensors = registry.discoverSensors();
    
    std::cout << "Found " << sensors.size() << " working sensors:" << std::endl;
    for (const auto& sensor : sensors) {
        std::cout << "  Port: " << sensor.port 
                  << ", Type: " << sensor.type 
                  << ", Available: " << (sensor.available ? "Yes" : "No") << std::endl;
    }
    
    std::cout << "
Discovering all devices..." << std::endl;
    auto allDevices = registry.discoverAllDevices();
    
    std::cout << "Found " << allDevices.size() << " total devices:" << std::endl;
    for (const auto& device : allDevices) {
        std::cout << "  Port: " << device.port 
                  << ", Type: " << device.type 
                  << ", Available: " << (device.available ? "Yes" : "No")
                  << ", Permissions: " << device.device_perms.getPermissionString() << std::endl;
    }
    
    return 0;
}

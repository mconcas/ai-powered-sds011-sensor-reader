#include "sensor_registry.h"
#include "sds011_plugin.h"
#include <iostream>

int main() {
    std::cout << "Testing main sensor discovery..." << std::endl;
    
    SensorRegistry registry;
    registry.registerPlugin(std::unique_ptr<SensorPlugin>(new SDS011Plugin()));
    
    std::cout << "Discovering sensors..." << std::endl;
    auto sensors = registry.discoverSensors();
    
    std::cout << "Found " << sensors.size() << " sensors:" << std::endl;
    for (const auto& sensor : sensors) {
        std::cout << "  Port: " << sensor.port << std::endl;
        std::cout << "  Type: " << sensor.type << std::endl;
        std::cout << "  Available: " << (sensor.available ? "Yes" : "No") << std::endl;
        std::cout << "  Permissions: " << sensor.device_perms.getPermissionString() << std::endl;
        std::cout << "  Status: " << sensor.device_perms.getStatusString() << std::endl;
        std::cout << "  Owner: " << sensor.device_perms.owner << ":" << sensor.device_perms.group << std::endl;
        std::cout << std::endl;
    }
    
    std::cout << "Discovering all devices..." << std::endl;
    auto allDevices = registry.discoverAllDevices();
    
    std::cout << "Found " << allDevices.size() << " devices total:" << std::endl;
    for (const auto& device : allDevices) {
        std::cout << "  Port: " << device.port << std::endl;
        std::cout << "  Type: " << device.type << std::endl;
        std::cout << "  Available: " << (device.available ? "Yes" : "No") << std::endl;
        std::cout << "  Permissions: " << device.device_perms.getPermissionString() << std::endl;
        std::cout << "  Status: " << device.device_perms.getStatusString() << std::endl;
        std::cout << std::endl;
    }
    
    return 0;
}

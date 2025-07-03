#include "sensor_registry.h"
#include "sds011_plugin.h"
#include <iostream>

int main() {
    std::cout << "Testing enhanced device discovery with permission checking..." << std::endl;
    
    SensorRegistry registry;
    registry.registerPlugin(std::unique_ptr<SensorPlugin>(new SDS011Plugin()));
    
    std::cout << "\nDiscovering all devices with detailed permissions..." << std::endl;
    auto allDevices = registry.discoverAllDevices();
    
    if (allDevices.empty()) {
        std::cout << "No devices found." << std::endl;
    } else {
        std::cout << "Found " << allDevices.size() << " device(s):" << std::endl;
        std::cout << std::endl;
        
        printf("%-20s %-12s %-12s %-15s %-12s %s\n", 
               "Port", "Permissions", "Owner:Group", "Access Status", "Available", "Error");
        printf("%s\n", std::string(80, '-').c_str());
        
        for (const auto& device : allDevices) {
            std::string owner_group = device.device_perms.owner + ":" + device.device_perms.group;
            std::string available_str = device.available ? "Yes" : "No";
            std::string error_msg = device.device_perms.error_message.empty() ? 
                                  "OK" : device.device_perms.error_message;
            
            printf("%-20s %-12s %-12s %-15s %-12s %s\n",
                   device.port.c_str(),
                   device.device_perms.getPermissionString().c_str(),
                   owner_group.c_str(),
                   device.device_perms.getStatusString().c_str(),
                   available_str.c_str(),
                   error_msg.c_str());
        }
    }
    
    std::cout << "\nTesting individual permission checking..." << std::endl;
    
    // Test common device paths
    std::vector<std::string> testPorts = {
        "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyACM0", "/dev/ttyACM1",
        "/dev/cu.usbserial", "/dev/cu.usbmodem", "/dev/null"
    };
    
    for (const auto& port : testPorts) {
        auto perms = SensorRegistry::checkDevicePermissions(port);
        std::cout << "Port " << port << ": ";
        if (!perms.exists) {
            std::cout << "Not found" << std::endl;
        } else {
            std::cout << perms.getPermissionString() << " (" 
                     << perms.owner << ":" << perms.group << ") - " 
                     << perms.getStatusString();
            if (!perms.error_message.empty()) {
                std::cout << " [" << perms.error_message << "]";
            }
            std::cout << std::endl;
        }
    }
    
    return 0;
}

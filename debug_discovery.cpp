#include "sensor_registry.h"
#include "sds011_plugin.h"
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>

int main() {
    std::cout << "Testing device discovery step by step..." << std::endl;
    
    // First, let's see what's actually in /dev
    std::cout << "\nListing all devices in /dev starting with 'cu.' or 'tty.':" << std::endl;
    DIR* dev_dir = opendir("/dev");
    if (dev_dir) {
        struct dirent* entry;
        while ((entry = readdir(dev_dir)) != nullptr) {
            std::string device_name = entry->d_name;
            
            if (device_name.find("cu.") == 0 || device_name.find("tty.") == 0) {
                std::string full_path = "/dev/" + device_name;
                struct stat st;
                bool is_char_device = (stat(full_path.c_str(), &st) == 0 && S_ISCHR(st.st_mode));
                
                std::cout << "  " << device_name << " -> " << full_path 
                          << " (char device: " << (is_char_device ? "yes" : "no") << ")" << std::endl;
                
                // Test our pattern matching
                bool matches_pattern = false;
                if (device_name.find("usbserial") != std::string::npos ||
                    device_name.find("usbmodem") != std::string::npos) {
                    matches_pattern = true;
                }
                else if ((device_name.find("cu.usb") == 0 || device_name.find("tty.usb") == 0) &&
                         device_name.length() > 6) {
                    matches_pattern = true;
                }
                
                std::cout << "    Pattern match: " << (matches_pattern ? "YES" : "NO") << std::endl;
            }
        }
        closedir(dev_dir);
    }
    
    std::cout << "\nTesting SDS011 known device patterns:" << std::endl;
    auto patterns = SDS011Plugin::getKnownDevicePatterns();
    for (const auto& pattern : patterns) {
        if (pattern.find('*') == std::string::npos) {
            struct stat st;
            bool exists = (stat(pattern.c_str(), &st) == 0 && S_ISCHR(st.st_mode));
            std::cout << "  " << pattern << ": " << (exists ? "EXISTS" : "not found") << std::endl;
        }
    }
    
    std::cout << "\nFinal discovery result:" << std::endl;
    auto devices = SensorRegistry::discoverSerialDevices();
    for (const auto& device : devices) {
        std::cout << "  " << device << std::endl;
    }
    
    return 0;
}

#include "sensor_registry.h"
#include "sds011_plugin.h"
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

// We'll replicate the exact function but with debug output
std::vector<std::string> debugDiscoverSerialDevices() {
    std::vector<std::string> serial_devices;
    
    std::cout << "Starting dynamic discovery by scanning /dev..." << std::endl;
    
    // First, try dynamic discovery by scanning /dev
    DIR* dev_dir = opendir("/dev");
    if (dev_dir) {
        struct dirent* entry;
        while ((entry = readdir(dev_dir)) != nullptr) {
            std::string device_name = entry->d_name;
            
            // Skip . and .. entries
            if (device_name == "." || device_name == "..") {
                continue;
            }
            
            // Check for serial device patterns
            bool is_serial_device = false;
            
#ifdef MACOS
            // macOS serial device patterns
            if (device_name.find("cu.") == 0 || device_name.find("tty.") == 0) {
                std::cout << "  Checking device: " << device_name << std::endl;
                
                // Common USB serial patterns - much more comprehensive
                if (device_name.find("usbserial") != std::string::npos ||
                    device_name.find("usbmodem") != std::string::npos ||
                    device_name.find("SLAB_USBtoUART") != std::string::npos ||
                    device_name.find("wchusbserial") != std::string::npos ||
                    device_name.find("CH34") != std::string::npos ||
                    device_name.find("CP210") != std::string::npos ||
                    device_name.find("FT") != std::string::npos ||
                    device_name.find("PL2303") != std::string::npos ||
                    device_name.find("Bluetooth") != std::string::npos) {
                    is_serial_device = true;
                    std::cout << "    -> Matches known patterns" << std::endl;
                }
                // Also include any cu. or tty. device that looks like a USB serial device
                // This catches devices like tty.usbserial-1140, cu.usbserial-A1B2C3, etc.
                else if ((device_name.find("cu.usb") == 0 || device_name.find("tty.usb") == 0) &&
                         device_name.length() > 6) {
                    is_serial_device = true;
                    std::cout << "    -> Matches USB device pattern" << std::endl;
                }
                
                if (!is_serial_device) {
                    std::cout << "    -> Does not match any pattern" << std::endl;
                }
            }
#endif
            
            if (is_serial_device) {
                std::string full_path = "/dev/" + device_name;
                
                // Verify it's actually a character device
                struct stat st;
                if (stat(full_path.c_str(), &st) == 0 && S_ISCHR(st.st_mode)) {
                    std::cout << "    -> Adding to list: " << full_path << std::endl;
                    serial_devices.push_back(full_path);
                } else {
                    std::cout << "    -> Not a character device or stat failed" << std::endl;
                }
            }
        }
        closedir(dev_dir);
    } else {
        std::cerr << "Warning: Could not open /dev directory" << std::endl;
    }
    
    std::cout << "Found " << serial_devices.size() << " devices from scanning" << std::endl;
    
    // Also check SDS011-specific known device patterns
    std::cout << "Checking SDS011 known device patterns..." << std::endl;
    auto sds011_patterns = SDS011Plugin::getKnownDevicePatterns();
    for (const auto& pattern : sds011_patterns) {
        // Skip wildcard patterns, only check specific device names
        if (pattern.find('*') == std::string::npos) {
            struct stat st;
            if (stat(pattern.c_str(), &st) == 0 && S_ISCHR(st.st_mode)) {
                // Add if not already in the list
                if (std::find(serial_devices.begin(), serial_devices.end(), pattern) == serial_devices.end()) {
                    std::cout << "  Adding known device: " << pattern << std::endl;
                    serial_devices.push_back(pattern);
                }
            }
        }
    }
    
    // Sort the devices for consistent ordering
    std::sort(serial_devices.begin(), serial_devices.end());
    
    std::cout << "Total devices found: " << serial_devices.size() << std::endl;
    
    // If no devices found dynamically, fall back to common ports
    if (serial_devices.empty()) {
        std::cout << "No devices found, falling back to common ports" << std::endl;
        return SensorRegistry::getCommonPorts();
    }
    
    return serial_devices;
}

int main() {
    std::cout << "Debug discovery with detailed output..." << std::endl;
    
    auto devices = debugDiscoverSerialDevices();
    
    std::cout << "\nFinal result:" << std::endl;
    for (const auto& device : devices) {
        std::cout << "  " << device << std::endl;
    }
    
    return 0;
}

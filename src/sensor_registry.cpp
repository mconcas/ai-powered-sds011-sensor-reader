#include "sensor_registry.h"
#include "sds011_plugin.h"
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <cstring>
#include <dirent.h>
#include <algorithm>

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
    std::vector<std::string> ports = discoverSerialDevices(); // Use dynamic discovery
    
    for (const auto& port : ports) {
        // Check if port exists
        struct stat st;
        if (stat(port.c_str(), &st) != 0) {
            continue;
        }
        
        // Get device permissions
        DevicePermissions device_perms = checkDevicePermissions(port);
        
        // Only test accessibility if we have proper permissions
        if (device_perms.readable && device_perms.writable) {
            // Only check devices that are likely SDS011 sensors to avoid false positives
            if (isLikelySDS011Device(port)) {
                // Test each plugin type
                for (const auto& pair : plugins) {
                    const std::string& type = pair.first;
                    auto plugin = createPlugin(type);
                    
                    if (plugin && plugin->isAvailable(port)) {
                        SensorInfo info(port, type, plugin->getDescription(), true);
                        info.device_perms = device_perms;
                        sensors.push_back(info);
                        break; // Port taken by this sensor type
                    }
                }
            }
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

DevicePermissions SensorRegistry::checkDevicePermissions(const std::string& port) {
    DevicePermissions perms;
    
    struct stat st;
    if (stat(port.c_str(), &st) != 0) {
        perms.exists = false;
        perms.error_message = "Device does not exist: " + std::string(strerror(errno));
        return perms;
    }
    
    perms.exists = true;
    perms.permissions = st.st_mode;
    
    // Get owner and group information
    struct passwd* pw = getpwuid(st.st_uid);
    if (pw) {
        perms.owner = pw->pw_name;
    } else {
        perms.owner = std::to_string(st.st_uid);
    }
    
    struct group* gr = getgrgid(st.st_gid);
    if (gr) {
        perms.group = gr->gr_name;
    } else {
        perms.group = std::to_string(st.st_gid);
    }
    
    // Check current user's access rights
    uid_t current_uid = getuid();
    gid_t current_gid = getgid();
    
    // Check read permission
    if (current_uid == st.st_uid) {
        // Owner permissions
        perms.readable = (st.st_mode & S_IRUSR) != 0;
        perms.writable = (st.st_mode & S_IWUSR) != 0;
    } else if (current_gid == st.st_gid) {
        // Group permissions
        perms.readable = (st.st_mode & S_IRGRP) != 0;
        perms.writable = (st.st_mode & S_IWGRP) != 0;
    } else {
        // Other permissions
        perms.readable = (st.st_mode & S_IROTH) != 0;
        perms.writable = (st.st_mode & S_IWOTH) != 0;
    }
    
    // Additional group membership check
    if (!perms.readable || !perms.writable) {
        // Check if user is in additional groups
        int ngroups = 0;
        struct passwd* current_user = getpwuid(current_uid);
        if (current_user) {
            getgrouplist(current_user->pw_name, current_gid, nullptr, &ngroups);
            if (ngroups > 0) {
                std::vector<int> groups(ngroups);
                if (getgrouplist(current_user->pw_name, current_gid, 
                               groups.data(), &ngroups) != -1) {
                    for (int gid : groups) {
                        if (static_cast<gid_t>(gid) == st.st_gid) {
                            perms.readable = (st.st_mode & S_IRGRP) != 0;
                            perms.writable = (st.st_mode & S_IWGRP) != 0;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    // Generate error message if needed
    if (!perms.readable && !perms.writable) {
        perms.error_message = "No read/write access. Current permissions: " + 
                             std::to_string((st.st_mode & 0777));
    } else if (!perms.readable) {
        perms.error_message = "No read access. Current permissions: " + 
                             std::to_string((st.st_mode & 0777));
    } else if (!perms.writable) {
        perms.error_message = "No write access. Current permissions: " + 
                             std::to_string((st.st_mode & 0777));
    }
    
    return perms;
}

std::vector<SensorInfo> SensorRegistry::discoverAllDevices() const {
    std::vector<SensorInfo> sensors;
    std::vector<std::string> ports = discoverSerialDevices(); // Use dynamic discovery
    
    for (const auto& port : ports) {
        // Check device permissions first
        DevicePermissions device_perms = checkDevicePermissions(port);
        
        if (!device_perms.exists) {
            continue; // Skip non-existent devices
        }
        
        bool sensor_detected = false;
        
        // Only test accessibility if we have proper permissions
        if (device_perms.readable && device_perms.writable) {
            // Only check devices that are likely SDS011 sensors to avoid false positives
            if (isLikelySDS011Device(port)) {
                // Test each plugin type
                for (const auto& pair : plugins) {
                    const std::string& type = pair.first;
                    auto plugin = createPlugin(type);
                    
                    if (plugin && plugin->isAvailable(port)) {
                        SensorInfo info(port, type, plugin->getDescription(), true);
                        info.device_perms = device_perms;
                        sensors.push_back(info);
                        sensor_detected = true;
                        break; // Port taken by this sensor type
                    }
                }
            }
        }
        
        // If no sensor detected, determine device type based on patterns
        if (!sensor_detected) {
            std::string device_type = "Unsupported";
            std::string description = "Unsupported device";
            
            // Check if this looks like a known SDS011 device pattern
            if (isLikelySDS011Device(port)) {
                device_type = "SDS011";
                description = "SDS011 PM Sensor (Not accessible)";
            }
            
            SensorInfo info(port, device_type, description, false);
            info.device_perms = device_perms;
            sensors.push_back(info);
        }
    }
    
    return sensors;
}

std::vector<std::string> SensorRegistry::discoverSerialDevices() {
    std::vector<std::string> serial_devices;
    
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
                }
                // Also include any cu. or tty. device that looks like a USB serial device
                // This catches devices like tty.usbserial-1140, cu.usbserial-A1B2C3, etc.
                else if ((device_name.find("cu.usb") == 0 || device_name.find("tty.usb") == 0) &&
                         device_name.length() > 6) {
                    is_serial_device = true;
                }
            }
#else
            // Linux serial device patterns
            if (device_name.find("ttyUSB") == 0 ||
                device_name.find("ttyACM") == 0 ||
                device_name.find("ttyS") == 0 ||
                device_name.find("ttyAMA") == 0) {
                is_serial_device = true;
            }
#endif
            
            if (is_serial_device) {
                std::string full_path = "/dev/" + device_name;
                
                // Verify it's actually a character device
                struct stat st;
                if (stat(full_path.c_str(), &st) == 0 && S_ISCHR(st.st_mode)) {
                    serial_devices.push_back(full_path);
                }
            }
        }
        closedir(dev_dir);
    } else {
        std::cerr << "Warning: Could not open /dev directory: " << strerror(errno) << std::endl;
    }
    
    // Also check SDS011-specific known device patterns
    auto sds011_patterns = SDS011Plugin::getKnownDevicePatterns();
    for (const auto& pattern : sds011_patterns) {
        // Skip wildcard patterns, only check specific device names
        if (pattern.find('*') == std::string::npos) {
            struct stat st;
            if (stat(pattern.c_str(), &st) == 0 && S_ISCHR(st.st_mode)) {
                // Add if not already in the list
                if (std::find(serial_devices.begin(), serial_devices.end(), pattern) == serial_devices.end()) {
                    serial_devices.push_back(pattern);
                }
            }
        }
    }
    
    // Sort the devices for consistent ordering
    std::sort(serial_devices.begin(), serial_devices.end());
    
    // If no devices found dynamically, fall back to common ports
    if (serial_devices.empty()) {
        return getCommonPorts();
    }
    
    return serial_devices;
}

bool SensorRegistry::isLikelySDS011Device(const std::string& port) const {
    // Get known SDS011 device patterns
    auto sds011_patterns = SDS011Plugin::getKnownDevicePatterns();
    
    for (const auto& pattern : sds011_patterns) {
        // Simple wildcard matching for patterns like "/dev/cu.usbserial*"
        if (pattern.find('*') != std::string::npos) {
            std::string prefix = pattern.substr(0, pattern.find('*'));
            if (port.find(prefix) == 0) {
                return true;
            }
        } else {
            // Exact match
            if (port == pattern) {
                return true;
            }
        }
    }
    
    // Additional heuristics for common SDS011 device names
    std::string device_name = port.substr(port.find_last_of('/') + 1);
    
    // Common patterns that suggest USB serial devices that could be SDS011
    if (device_name.find("usbserial") != std::string::npos ||
        device_name.find("usbmodem") != std::string::npos ||
        device_name.find("SLAB_USBtoUART") != std::string::npos ||
        device_name.find("CH34") != std::string::npos ||
        device_name.find("CP210") != std::string::npos) {
        return true;
    }
    
    return false;
}

std::string DevicePermissions::getPermissionString() const {
    if (!exists) {
        return "--------";
    }
    
    std::string perm_str = "";
    // User permissions
    perm_str += (permissions & S_IRUSR) ? "r" : "-";
    perm_str += (permissions & S_IWUSR) ? "w" : "-";
    perm_str += (permissions & S_IXUSR) ? "x" : "-";
    
    // Group permissions
    perm_str += (permissions & S_IRGRP) ? "r" : "-";
    perm_str += (permissions & S_IWGRP) ? "w" : "-";
    perm_str += (permissions & S_IXGRP) ? "x" : "-";
    
    // Other permissions
    perm_str += (permissions & S_IROTH) ? "r" : "-";
    perm_str += (permissions & S_IWOTH) ? "w" : "-";
    perm_str += (permissions & S_IXOTH) ? "x" : "-";
    
    return perm_str;
}

std::string DevicePermissions::getStatusString() const {
    if (!exists) {
        return "Not Found";
    }
    
    if (readable && writable) {
        return "R/W Access";
    } else if (readable) {
        return "Read Only";
    } else if (writable) {
        return "Write Only";
    } else {
        return "No Access";
    }
}

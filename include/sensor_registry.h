#pragma once

#include "sensor_plugin.h"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <sys/stat.h>

/**
 * @brief Device permission information
 */
struct DevicePermissions {
    bool exists;
    bool readable;
    bool writable;
    std::string owner;
    std::string group;
    mode_t permissions;
    std::string error_message;
    
    DevicePermissions() : exists(false), readable(false), writable(false), permissions(0) {}
    
    /**
     * @brief Get formatted permission string (e.g., "rw-rw----")
     */
    std::string getPermissionString() const;
    
    /**
     * @brief Get human-readable status
     */
    std::string getStatusString() const;
};

/**
 * @brief Sensor information for discovery
 */
struct SensorInfo {
    std::string port;
    std::string type;
    std::string description;
    bool available;
    DevicePermissions device_perms;
    
    SensorInfo(const std::string& p, const std::string& t, 
               const std::string& d, bool a)
        : port(p), type(t), description(d), available(a) {}
};

/**
 * @brief Registry for sensor plugins
 */
class SensorRegistry {
private:
    std::map<std::string, std::unique_ptr<SensorPlugin>> plugins;
    
public:
    /**
     * @brief Register a sensor plugin
     */
    void registerPlugin(std::unique_ptr<SensorPlugin> plugin);
    
    /**
     * @brief Get all registered plugin types
     */
    std::vector<std::string> getAvailableTypes() const;
    
    /**
     * @brief Create a plugin instance by type
     */
    std::unique_ptr<SensorPlugin> createPlugin(const std::string& type) const;
    
    /**
     * @brief Discover available sensors on all common ports
     */
    std::vector<SensorInfo> discoverSensors() const;
    
    /**
     * @brief Discover all devices with detailed permission information
     */
    std::vector<SensorInfo> discoverAllDevices() const;
    
    /**
     * @brief Check device permissions for a specific port
     */
    static DevicePermissions checkDevicePermissions(const std::string& port);
    
    /**
     * @brief Get common serial ports to scan
     */
    static std::vector<std::string> getCommonPorts();
    
    /**
     * @brief Dynamically discover all serial devices in /dev
     */
    static std::vector<std::string> discoverSerialDevices();

private:
    /**
     * @brief Check if a device port is likely to be an SDS011 sensor
     */
    bool isLikelySDS011Device(const std::string& port) const;
};

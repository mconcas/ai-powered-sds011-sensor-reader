#pragma once

#include "sensor_plugin.h"
#include <memory>
#include <vector>
#include <map>

/**
 * @brief Sensor information for discovery
 */
struct SensorInfo {
    std::string port;
    std::string type;
    std::string description;
    bool available;
    
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
     * @brief Get common serial ports to scan
     */
    static std::vector<std::string> getCommonPorts();
};

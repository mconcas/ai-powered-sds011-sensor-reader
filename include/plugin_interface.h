#pragma once

#include <string>
#include <vector>
#include <memory>
#include <ncurses.h>

/**
 * @brief Device information structure
 */
struct DeviceInfo {
    std::string port;
    std::string vendor_id;
    std::string product_id;
    std::string description;
    bool accessible;
    
    // Equality operator for robust duplicate detection
    bool operator==(const DeviceInfo& other) const {
        return port == other.port && 
               vendor_id == other.vendor_id && 
               product_id == other.product_id;
    }
};

/**
 * @brief Base class for sensor data
 */
class SensorData {
public:
    virtual ~SensorData() = default;
    virtual std::string getDisplayString() const = 0;
    virtual std::string getQualityDescription() const = 0;
    virtual int getColorCode() const = 0;
};

/**
 * @brief Plugin UI interface
 */
class PluginUI {
public:
    virtual ~PluginUI() = default;
    
    // UI Management
    virtual bool initialize(int maxY, int maxX) = 0;
    virtual void cleanup() = 0;
    virtual void createWindows() = 0;
    virtual void resize(int maxY, int maxX) = 0;
    
    // Display Functions
    virtual void showHeader(const std::string& port, const std::string& status) = 0;
    virtual void updateDataDisplay(const std::vector<std::unique_ptr<SensorData>>& readings) = 0;
    virtual void updateStatistics(const std::vector<std::unique_ptr<SensorData>>& readings) = 0;
    virtual void showError(const std::string& message) = 0;
    virtual void showStatus(const std::string& status) = 0;
    
    // Input Handling
    virtual int handleInput() = 0; // Returns: 0=continue, 1=quit, 2=back to menu
    
    // Plugin Info
    virtual std::string getPluginName() const = 0;
    virtual std::string getVersion() const = 0;
};

/**
 * @brief Plugin sensor interface
 */
class PluginSensor {
public:
    virtual ~PluginSensor() = default;
    
    // Sensor Management
    virtual bool initialize(const std::string& port) = 0;
    virtual void cleanup() = 0;
    virtual bool isConnected() const = 0;
    
    // Data Operations
    virtual std::unique_ptr<SensorData> readData() = 0;
    virtual bool calibrate() = 0;
    virtual void reset() = 0;
    
    // Sensor Info
    virtual std::string getSensorName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::vector<std::string> getSupportedDevices() const = 0;
};

/**
 * @brief Main plugin interface
 */
class Plugin {
public:
    virtual ~Plugin() = default;
    
    // Plugin Lifecycle
    virtual bool initialize() = 0;
    virtual void cleanup() = 0;
    
    // Device Detection
    virtual std::vector<DeviceInfo> detectDevices() const = 0;
    virtual bool canHandleDevice(const DeviceInfo& device) const = 0;
    virtual double getDeviceMatchScore(const DeviceInfo& device) const = 0;
    
    // Factory Methods
    virtual std::unique_ptr<PluginSensor> createSensor() = 0;
    virtual std::unique_ptr<PluginUI> createUI() = 0;
    
    // Plugin Info
    virtual std::string getPluginName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::vector<std::string> getSupportedDevicePatterns() const = 0;
};

// Plugin entry points (C interface for dynamic loading)
extern "C" {
    typedef Plugin* (*CreatePluginFunc)();
    typedef void (*DestroyPluginFunc)(Plugin*);
    typedef const char* (*GetPluginNameFunc)();
    typedef const char* (*GetPluginVersionFunc)();
}

#define PLUGIN_API extern "C"
#define CREATE_PLUGIN_FUNC "createPlugin"
#define DESTROY_PLUGIN_FUNC "destroyPlugin"
#define GET_PLUGIN_NAME_FUNC "getPluginName"
#define GET_PLUGIN_VERSION_FUNC "getPluginVersion"

#pragma once

#include <string>
#include <memory>
#include <map>
#include <vector>

/**
 * @brief Base class for sensor data
 */
class SensorData {
public:
    virtual ~SensorData() = default;
    virtual std::string toString() const = 0;
    virtual std::string getDisplayString() const = 0;
};

/**
 * @brief Base interface for sensor plugins
 */
class SensorPlugin {
public:
    virtual ~SensorPlugin() = default;
    
    /**
     * @brief Get the sensor type name
     */
    virtual std::string getTypeName() const = 0;
    
    /**
     * @brief Get human-readable description
     */
    virtual std::string getDescription() const = 0;
    
    /**
     * @brief Check if the sensor is available at the given port
     */
    virtual bool isAvailable(const std::string& port) const = 0;
    
    /**
     * @brief Initialize connection to the sensor
     */
    virtual bool initialize(const std::string& port) = 0;
    
    /**
     * @brief Read data from the sensor
     */
    virtual std::unique_ptr<SensorData> readData() = 0;
    
    /**
     * @brief Get the current port
     */
    virtual std::string getCurrentPort() const = 0;
    
    /**
     * @brief Get sensor-specific display headers
     */
    virtual std::vector<std::string> getDisplayHeaders() const = 0;
    
    /**
     * @brief Get color coding for data values (for TUI)
     * @param data The sensor data to evaluate
     * @return Color pair number (1=green, 2=yellow, 3=red)
     */
    virtual int getColorCode(const SensorData& data) const = 0;
    
    /**
     * @brief Get quality description for data
     */
    virtual std::string getQualityDescription(const SensorData& data) const = 0;
    
    /**
     * @brief Cleanup resources
     */
    virtual void cleanup() = 0;
};

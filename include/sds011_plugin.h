#pragma once

#include "sensor_plugin.h"
#include <chrono>

/**
 * @brief SDS011 specific sensor data
 */
class SDS011Data : public SensorData {
public:
    float pm25;
    float pm10;
    std::chrono::system_clock::time_point timestamp;
    
    SDS011Data(float p25, float p10) 
        : pm25(p25), pm10(p10), timestamp(std::chrono::system_clock::now()) {}
    
    std::string toString() const override;
    std::string getDisplayString() const override;
};

/**
 * @brief SDS011 PM2.5 Sensor Plugin
 */
class SDS011Plugin : public SensorPlugin {
private:
    int serial_fd;
    std::string current_port;
    
    // SDS011 Protocol constants
    static const unsigned char HEADER = 0xAA;
    static const unsigned char TAIL = 0xAB;
    static const unsigned char CMD_ID = 0xC0;
    static const int DATA_LENGTH = 10;
    
    /**
     * @brief Read a raw packet from the sensor
     */
    bool readPacket(std::vector<unsigned char>& packet);
    
    /**
     * @brief Setup serial port configuration
     */
    bool configureSerialPort();
    
public:
    SDS011Plugin();
    ~SDS011Plugin();
    
    // SensorPlugin interface
    std::string getTypeName() const override { return "SDS011"; }
    std::string getDescription() const override { return "SDS011 PM2.5/PM10 Particulate Matter Sensor"; }
    bool isAvailable(const std::string& port) const override;
    bool initialize(const std::string& port) override;
    std::unique_ptr<SensorData> readData() override;
    std::string getCurrentPort() const override { return current_port; }
    std::vector<std::string> getDisplayHeaders() const override;
    int getColorCode(const SensorData& data) const override;
    std::string getQualityDescription(const SensorData& data) const override;
    void cleanup() override;
    
    /**
     * @brief Get known device patterns that are compatible with SDS011
     */
    static std::vector<std::string> getKnownDevicePatterns();
};

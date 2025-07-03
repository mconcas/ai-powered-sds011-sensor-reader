#pragma once

#include <string>
#include <vector>

/**
 * @brief SDS011 PM2.5 Sensor Reader Class
 * 
 * This class provides an interface to read particulate matter data from
 * the SDS011 PM2.5 sensor via serial communication.
 */
class SDS011Reader {
private:
    int serial_fd;
    std::string port_name;
    
    // SDS011 Protocol constants
    static const unsigned char HEADER = 0xAA;
    static const unsigned char TAIL = 0xAB;
    static const unsigned char CMD_ID = 0xC0;
    static const int DATA_LENGTH = 10;
    
    /**
     * @brief Read a raw packet from the sensor
     * @param packet Vector to store the received packet
     * @return true if a valid packet was received, false otherwise
     */
    bool readPacket(std::vector<unsigned char>& packet);
    
public:
    /**
     * @brief Constructor
     * @param port Serial port device path (default: /dev/ttyUSB0)
     */
    SDS011Reader(const std::string& port = "/dev/ttyUSB0");
    
    /**
     * @brief Destructor - closes serial port if open
     */
    ~SDS011Reader();
    
    /**
     * @brief Initialize the serial connection to the sensor
     * @return true if initialization successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Read PM2.5 and PM10 data from the sensor
     * @param pm25 Reference to store PM2.5 value (µg/m³)
     * @param pm10 Reference to store PM10 value (µg/m³)
     * @return true if data was successfully read, false otherwise
     */
    bool readPM25Data(float& pm25, float& pm10);
    
    /**
     * @brief Print raw packet data in hexadecimal format (for debugging)
     * @param packet The packet to print
     */
    void printPacketHex(const std::vector<unsigned char>& packet);
    
    /**
     * @brief Get the current port name
     * @return The serial port device path
     */
    const std::string& getPortName() const { return port_name; }
};

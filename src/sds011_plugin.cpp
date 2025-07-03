#include "sds011_plugin.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

// SDS011Data implementation
std::string SDS011Data::toString() const {
    std::ostringstream oss;
    oss << "PM2.5: " << std::fixed << std::setprecision(1) << pm25 
        << " µg/m³, PM10: " << pm10 << " µg/m³";
    return oss.str();
}

std::string SDS011Data::getDisplayString() const {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
        << std::setw(2) << tm.tm_min << ":" << std::setw(2) << tm.tm_sec
        << "   " << std::fixed << std::setprecision(1) 
        << std::setw(8) << pm25 << "   " << std::setw(8) << pm10;
    return oss.str();
}

// SDS011Plugin implementation
SDS011Plugin::SDS011Plugin() : serial_fd(-1) {}

SDS011Plugin::~SDS011Plugin() {
    cleanup();
}

bool SDS011Plugin::isAvailable(const std::string& port) const {
    // Try to open the port briefly to check availability
    int test_fd = open(port.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (test_fd < 0) {
        return false;
    }
    
    // Basic port test - just check if we can configure it
    struct termios tty;
    bool available = (tcgetattr(test_fd, &tty) == 0);
    
    close(test_fd);
    return available;
}

bool SDS011Plugin::initialize(const std::string& port) {
    cleanup(); // Close any existing connection
    
    current_port = port;
    
    // Open serial port
    serial_fd = open(port.c_str(), O_RDONLY | O_NOCTTY | O_SYNC);
    if (serial_fd < 0) {
        return false;
    }
    
    return configureSerialPort();
}

bool SDS011Plugin::configureSerialPort() {
    struct termios tty;
    if (tcgetattr(serial_fd, &tty) != 0) {
        return false;
    }
    
    // Set baud rate to 9600 (SDS011 default)
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    
    // Configure 8N1 (8 data bits, no parity, 1 stop bit)
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                         // disable break processing
    tty.c_lflag = 0;                                // no signaling chars, no echo,
                                                    // no canonical processing
    tty.c_oflag = 0;                                // no remapping, no delays
    tty.c_cc[VMIN] = 0;                             // read doesn't block
    tty.c_cc[VTIME] = 5;                            // 0.5 seconds read timeout
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // shut off xon/xoff ctrl
    
    tty.c_cflag |= (CLOCAL | CREAD);                // ignore modem controls,
                                                    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);              // shut off parity
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    
    return (tcsetattr(serial_fd, TCSANOW, &tty) == 0);
}

bool SDS011Plugin::readPacket(std::vector<unsigned char>& packet) {
    packet.clear();
    packet.resize(DATA_LENGTH);
    
    // Read data from serial port
    int bytes_read = read(serial_fd, packet.data(), DATA_LENGTH);
    if (bytes_read != DATA_LENGTH) {
        return false;
    }
    
    // Validate packet structure
    if (packet[0] != HEADER || packet[9] != TAIL || packet[1] != CMD_ID) {
        return false;
    }
    
    // Validate checksum
    unsigned char checksum = 0;
    for (int i = 2; i < 8; i++) {
        checksum += packet[i];
    }
    
    if (checksum != packet[8]) {
        return false;
    }
    
    return true;
}

std::unique_ptr<SensorData> SDS011Plugin::readData() {
    if (serial_fd < 0) {
        return nullptr;
    }
    
    std::vector<unsigned char> packet;
    
    // Try to read valid packet (may need multiple attempts)
    for (int attempts = 0; attempts < 10; attempts++) {
        if (readPacket(packet)) {
            // Extract PM2.5 and PM10 values
            // Data is in little-endian format
            int pm25_raw = packet[2] | (packet[3] << 8);
            int pm10_raw = packet[4] | (packet[5] << 8);
            
            // Convert to µg/m³ (divide by 10 as per SDS011 specification)
            float pm25 = pm25_raw / 10.0f;
            float pm10 = pm10_raw / 10.0f;
            
            return std::unique_ptr<SensorData>(new SDS011Data(pm25, pm10));
        }
        
        // Small delay before retry
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return nullptr;
}

std::vector<std::string> SDS011Plugin::getDisplayHeaders() const {
    return {"Time", "PM2.5 (µg/m³)", "PM10 (µg/m³)", "Quality"};
}

int SDS011Plugin::getColorCode(const SensorData& data) const {
    const SDS011Data* sds_data = dynamic_cast<const SDS011Data*>(&data);
    if (!sds_data) return 1;
    
    // Color based on PM2.5 levels (WHO guidelines)
    if (sds_data->pm25 <= 15.0) return 1; // Green (good)
    if (sds_data->pm25 <= 25.0) return 2; // Yellow (moderate)
    return 3; // Red (poor)
}

std::string SDS011Plugin::getQualityDescription(const SensorData& data) const {
    const SDS011Data* sds_data = dynamic_cast<const SDS011Data*>(&data);
    if (!sds_data) return "Unknown";
    
    if (sds_data->pm25 <= 15.0) return "Good";
    if (sds_data->pm25 <= 25.0) return "Moderate";
    return "Poor";
}

void SDS011Plugin::cleanup() {
    if (serial_fd >= 0) {
        close(serial_fd);
        serial_fd = -1;
    }
    current_port.clear();
}

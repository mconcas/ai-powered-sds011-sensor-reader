#include "sds011_reader.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

SDS011Reader::SDS011Reader(const std::string& port) : serial_fd(-1), port_name(port) {}

SDS011Reader::~SDS011Reader() {
    if (serial_fd >= 0) {
        close(serial_fd);
    }
}

bool SDS011Reader::initialize() {
    // Open serial port
    serial_fd = open(port_name.c_str(), O_RDONLY | O_NOCTTY | O_SYNC);
    if (serial_fd < 0) {
        std::cerr << "Error opening serial port: " << port_name << std::endl;
        return false;
    }
    
    // Configure serial port
    struct termios tty;
    if (tcgetattr(serial_fd, &tty) != 0) {
        std::cerr << "Error getting terminal attributes" << std::endl;
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
    
    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        std::cerr << "Error setting terminal attributes" << std::endl;
        return false;
    }
    
    std::cout << "Serial port " << port_name << " initialized successfully" << std::endl;
    return true;
}

bool SDS011Reader::readPacket(std::vector<unsigned char>& packet) {
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

bool SDS011Reader::readPM25Data(float& pm25, float& pm10) {
    std::vector<unsigned char> packet;
    
    // Try to read valid packet (may need multiple attempts)
    for (int attempts = 0; attempts < 10; attempts++) {
        if (readPacket(packet)) {
            // Extract PM2.5 and PM10 values
            // Data is in little-endian format
            int pm25_raw = packet[2] | (packet[3] << 8);
            int pm10_raw = packet[4] | (packet[5] << 8);
            
            // Convert to µg/m³ (divide by 10 as per SDS011 specification)
            pm25 = pm25_raw / 10.0f;
            pm10 = pm10_raw / 10.0f;
            
            return true;
        }
        
        // Small delay before retry
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return false;
}

void SDS011Reader::printPacketHex(const std::vector<unsigned char>& packet) {
    std::cout << "Raw packet: ";
    for (size_t i = 0; i < packet.size(); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                 << static_cast<int>(packet[i]) << " ";
    }
    std::cout << std::dec << std::endl;
}

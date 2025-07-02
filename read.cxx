#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <ncurses.h>
#include <signal.h>
#include <deque>
#include <algorithm>

#include <deque>
#include <algorithm>

// Structure to store sensor readings
struct SensorReading {
    std::chrono::system_clock::time_point timestamp;
    float pm25;
    float pm10;
    
    SensorReading(float p25, float p10) 
        : timestamp(std::chrono::system_clock::now()), pm25(p25), pm10(p10) {}
};

// Global flag for clean shutdown
volatile bool g_running = true;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_running = false;
    }
}

class SDS011TUI {
private:
    WINDOW* mainWin;
    WINDOW* headerWin;
    WINDOW* dataWin;
    WINDOW* statsWin;
    WINDOW* statusWin;
    
    std::deque<SensorReading> readings;
    static const size_t MAX_READINGS = 100;
    
    int maxY, maxX;
    
public:
    SDS011TUI() : mainWin(nullptr), headerWin(nullptr), dataWin(nullptr), 
                  statsWin(nullptr), statusWin(nullptr) {}
    
    ~SDS011TUI() {
        cleanup();
    }
    
    bool initialize() {
        // Initialize ncurses
        mainWin = initscr();
        if (mainWin == nullptr) {
            return false;
        }
        
        // Configure ncurses
        cbreak();           // Disable line buffering
        noecho();           // Don't echo pressed keys
        keypad(stdscr, TRUE); // Enable special keys
        curs_set(0);        // Hide cursor
        timeout(100);       // Non-blocking getch with 100ms timeout
        
        // Check if colors are supported
        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_GREEN, COLOR_BLACK);   // Good values
            init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // Moderate values
            init_pair(3, COLOR_RED, COLOR_BLACK);     // High values
            init_pair(4, COLOR_CYAN, COLOR_BLACK);    // Headers
            init_pair(5, COLOR_WHITE, COLOR_BLUE);    // Status bar
        }
        
        getmaxyx(stdscr, maxY, maxX);
        
        // Create windows
        createWindows();
        
        return true;
    }
    
    void createWindows() {
        // Header window (top 3 lines)
        headerWin = newwin(3, maxX, 0, 0);
        
        // Data window (main area)
        dataWin = newwin(maxY - 8, maxX, 3, 0);
        
        // Statistics window (right side, bottom)
        statsWin = newwin(3, maxX / 2, maxY - 5, maxX / 2);
        
        // Status window (bottom 2 lines)
        statusWin = newwin(2, maxX, maxY - 2, 0);
        
        // Enable scrolling for data window
        scrollok(dataWin, TRUE);
        
        // Draw borders
        box(headerWin, 0, 0);
        box(dataWin, 0, 0);
        box(statsWin, 0, 0);
        box(statusWin, 0, 0);
    }
    
    void cleanup() {
        if (headerWin) delwin(headerWin);
        if (dataWin) delwin(dataWin);
        if (statsWin) delwin(statsWin);
        if (statusWin) delwin(statusWin);
        
        if (mainWin) {
            endwin();
        }
    }
    
    void drawHeader(const std::string& port) {
        wclear(headerWin);
        box(headerWin, 0, 0);
        
        if (has_colors()) {
            wattron(headerWin, COLOR_PAIR(4) | A_BOLD);
        }
        
        mvwprintw(headerWin, 1, 2, "SDS011 PM2.5 Sensor Reader - TUI Mode");
        mvwprintw(headerWin, 2, 2, "Port: %s | Press 'q' to quit, 'c' to clear data", port.c_str());
        
        if (has_colors()) {
            wattroff(headerWin, COLOR_PAIR(4) | A_BOLD);
        }
        
        wrefresh(headerWin);
    }
    
    void addReading(float pm25, float pm10) {
        readings.emplace_back(pm25, pm10);
        
        // Keep only the last MAX_READINGS
        if (readings.size() > MAX_READINGS) {
            readings.pop_front();
        }
        
        // Update display
        updateDataWindow();
        updateStatsWindow();
        updateStatusWindow();
    }
    
    void updateDataWindow() {
        wclear(dataWin);
        box(dataWin, 0, 0);
        
        // Header
        if (has_colors()) {
            wattron(dataWin, COLOR_PAIR(4) | A_BOLD);
        }
        mvwprintw(dataWin, 1, 2, "%-10s %-12s %-12s %-8s", "Time", "PM2.5", "PM10", "Quality");
        mvwprintw(dataWin, 2, 2, "%s", std::string(maxX - 6, '-').c_str());
        if (has_colors()) {
            wattroff(dataWin, COLOR_PAIR(4) | A_BOLD);
        }
        
        // Display last readings (most recent first)
        int line = 3;
        int maxLines = maxY - 11; // Account for borders and other windows
        
        for (auto it = readings.rbegin(); it != readings.rend() && line < maxLines; ++it, ++line) {
            auto time_t = std::chrono::system_clock::to_time_t(it->timestamp);
            auto tm = *std::localtime(&time_t);
            
            // Color based on PM2.5 levels (WHO guidelines)
            int colorPair = 1; // Green (good)
            std::string quality = "Good";
            
            if (it->pm25 > 15.0) {
                colorPair = 2; // Yellow (moderate)
                quality = "Moderate";
            }
            if (it->pm25 > 25.0) {
                colorPair = 3; // Red (unhealthy)
                quality = "Poor";
            }
            
            if (has_colors()) {
                wattron(dataWin, COLOR_PAIR(colorPair));
            }
            
            mvwprintw(dataWin, line, 2, "%02d:%02d:%02d   %-8.1f   %-8.1f   %-8s",
                     tm.tm_hour, tm.tm_min, tm.tm_sec,
                     it->pm25, it->pm10, quality.c_str());
            
            if (has_colors()) {
                wattroff(dataWin, COLOR_PAIR(colorPair));
            }
        }
        
        wrefresh(dataWin);
    }
    
    void updateStatsWindow() {
        if (readings.empty()) return;
        
        wclear(statsWin);
        box(statsWin, 0, 0);
        
        // Calculate statistics
        float sumPM25 = 0, sumPM10 = 0;
        float minPM25 = readings[0].pm25, maxPM25 = readings[0].pm25;
        float minPM10 = readings[0].pm10, maxPM10 = readings[0].pm10;
        
        for (const auto& reading : readings) {
            sumPM25 += reading.pm25;
            sumPM10 += reading.pm10;
            minPM25 = std::min(minPM25, reading.pm25);
            maxPM25 = std::max(maxPM25, reading.pm25);
            minPM10 = std::min(minPM10, reading.pm10);
            maxPM10 = std::max(maxPM10, reading.pm10);
        }
        
        float avgPM25 = sumPM25 / readings.size();
        float avgPM10 = sumPM10 / readings.size();
        
        if (has_colors()) {
            wattron(statsWin, COLOR_PAIR(4) | A_BOLD);
        }
        mvwprintw(statsWin, 0, 2, "Statistics (last %zu readings)", readings.size());
        if (has_colors()) {
            wattroff(statsWin, COLOR_PAIR(4) | A_BOLD);
        }
        
        mvwprintw(statsWin, 1, 2, "PM2.5: Avg %.1f Min %.1f Max %.1f", avgPM25, minPM25, maxPM25);
        mvwprintw(statsWin, 2, 2, "PM10:  Avg %.1f Min %.1f Max %.1f", avgPM10, minPM10, maxPM10);
        
        wrefresh(statsWin);
    }
    
    void updateStatusWindow() {
        wclear(statusWin);
        box(statusWin, 0, 0);
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        if (has_colors()) {
            wattron(statusWin, COLOR_PAIR(5));
        }
        
        mvwprintw(statusWin, 1, 2, "Status: Running | Last update: %02d:%02d:%02d | Total readings: %zu",
                 tm.tm_hour, tm.tm_min, tm.tm_sec, readings.size());
        
        if (has_colors()) {
            wattroff(statusWin, COLOR_PAIR(5));
        }
        
        wrefresh(statusWin);
    }
    
    void showError(const std::string& message) {
        wclear(statusWin);
        box(statusWin, 0, 0);
        
        if (has_colors()) {
            wattron(statusWin, COLOR_PAIR(3) | A_BOLD);
        }
        
        mvwprintw(statusWin, 1, 2, "ERROR: %s", message.c_str());
        
        if (has_colors()) {
            wattroff(statusWin, COLOR_PAIR(3) | A_BOLD);
        }
        
        wrefresh(statusWin);
    }
    
    void clearData() {
        readings.clear();
        updateDataWindow();
        updateStatsWindow();
        updateStatusWindow();
    }
    
    int handleInput() {
        int ch = getch();
        switch (ch) {
            case 'q':
            case 'Q':
                return 1; // Quit
            case 'c':
            case 'C':
                clearData();
                break;
            case KEY_RESIZE:
                // Handle terminal resize
                getmaxyx(stdscr, maxY, maxX);
                wresize(headerWin, 3, maxX);
                wresize(dataWin, maxY - 8, maxX);
                wresize(statsWin, 3, maxX / 2);
                mvwin(statsWin, maxY - 5, maxX / 2);
                wresize(statusWin, 2, maxX);
                mvwin(statusWin, maxY - 2, 0);
                
                // Redraw everything
                updateDataWindow();
                updateStatsWindow();
                updateStatusWindow();
                break;
        }
        return 0;
    }
};

class SDS011Reader {
private:
    int serial_fd;
    std::string port_name;
    
    // SDS011 Protocol constants
    static const unsigned char HEADER = 0xAA;
    static const unsigned char TAIL = 0xAB;
    static const unsigned char CMD_ID = 0xC0;
    static const int DATA_LENGTH = 10;
    
public:
    SDS011Reader(const std::string& port = "/dev/ttyUSB0") : serial_fd(-1), port_name(port) {}
    
    ~SDS011Reader() {
        if (serial_fd >= 0) {
            close(serial_fd);
        }
    }
    
    bool initialize() {
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
    
    bool readPacket(std::vector<unsigned char>& packet) {
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
    
    bool readPM25Data(float& pm25, float& pm10) {
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
    
    void printPacketHex(const std::vector<unsigned char>& packet) {
        std::cout << "Raw packet: ";
        for (size_t i = 0; i < packet.size(); i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                     << static_cast<int>(packet[i]) << " ";
        }
        std::cout << std::dec << std::endl;
    }
};

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options] [serial_port]" << std::endl;
    std::cout << "  Options:" << std::endl;
    std::cout << "    --no-tui    Disable TUI mode and use console output" << std::endl;
    std::cout << "    -h, --help  Show this help message" << std::endl;
    std::cout << "  serial_port: Serial port device (default: /dev/ttyUSB0)" << std::endl;
    std::cout << std::endl;
    std::cout << "  TUI Controls:" << std::endl;
    std::cout << "    q    Quit the program" << std::endl;
    std::cout << "    c    Clear all collected data" << std::endl;
    std::cout << std::endl;
    std::cout << "  Examples:" << std::endl;
    std::cout << "    " << program_name << "                    # TUI mode with default port" << std::endl;
    std::cout << "    " << program_name << " /dev/ttyUSB1       # TUI mode with custom port" << std::endl;
    std::cout << "    " << program_name << " --no-tui           # Console mode with default port" << std::endl;
    std::cout << "    " << program_name << " --no-tui /dev/ttyACM0  # Console mode with custom port" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string serial_port = "/dev/ttyUSB0";
    bool use_tui = true;
    
    // Parse command line arguments
    if (argc > 1) {
        if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        if (std::string(argv[1]) == "--no-tui") {
            use_tui = false;
            if (argc > 2) {
                serial_port = argv[2];
            }
        } else {
            serial_port = argv[1];
        }
    }
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize sensor reader
    SDS011Reader sensor(serial_port);
    if (!sensor.initialize()) {
        std::cerr << "Failed to initialize sensor. Please check:" << std::endl;
        std::cerr << "  - Serial port exists and is accessible" << std::endl;
        std::cerr << "  - User has permission to access the port" << std::endl;
        std::cerr << "  - SDS011 sensor is connected and powered on" << std::endl;
        return 1;
    }
    
    if (use_tui) {
        // TUI Mode
        SDS011TUI tui;
        if (!tui.initialize()) {
            std::cerr << "Failed to initialize TUI. Falling back to console mode." << std::endl;
            use_tui = false;
        } else {
            tui.drawHeader(serial_port);
            
            // Main reading loop with TUI
            while (g_running) {
                float pm25, pm10;
                
                if (sensor.readPM25Data(pm25, pm10)) {
                    tui.addReading(pm25, pm10);
                } else {
                    tui.showError("Failed to read valid data from sensor");
                }
                
                // Handle user input
                if (tui.handleInput() == 1) {
                    break; // User pressed 'q'
                }
                
                // Wait before next reading
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            
            return 0;
        }
    }
    
    if (!use_tui) {
        // Console Mode (original functionality)
        std::cout << "SDS011 PM2.5 Sensor Reader - Console Mode" << std::endl;
        std::cout << "==========================================" << std::endl;
        std::cout << "Product model: SDS011 V1.3" << std::endl;
        std::cout << "Serial port: " << serial_port << std::endl;
        std::cout << "Use --no-tui to disable TUI mode" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Reading PM2.5 data (Press Ctrl+C to exit)..." << std::endl;
        std::cout << std::endl;
        
        // Print header
        std::cout << std::setw(20) << "Timestamp"
                  << std::setw(12) << "PM2.5 (µg/m³)"
                  << std::setw(12) << "PM10 (µg/m³)"
                  << std::endl;
        std::cout << std::string(44, '-') << std::endl;
        
        // Main reading loop
        int reading_count = 0;
        while (g_running) {
            float pm25, pm10;
            
            if (sensor.readPM25Data(pm25, pm10)) {
                // Get current timestamp
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                auto tm = *std::localtime(&time_t);
                
                // Print formatted data
                std::cout << std::setw(2) << std::setfill('0') << tm.tm_hour << ":"
                         << std::setw(2) << std::setfill('0') << tm.tm_min << ":"
                         << std::setw(2) << std::setfill('0') << tm.tm_sec
                         << std::setw(12) << std::setfill(' ') << std::fixed << std::setprecision(1) << pm25
                         << std::setw(12) << std::setprecision(1) << pm10
                         << std::endl;
                
                reading_count++;
                
                // Print summary every 10 readings
                if (reading_count % 10 == 0) {
                    std::cout << std::endl << "Readings collected: " << reading_count << std::endl;
                    std::cout << std::string(44, '-') << std::endl;
                }
            } else {
                std::cerr << "Failed to read valid data from sensor" << std::endl;
            }
            
            // Wait before next reading (SDS011 updates every ~1 second)
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    return 0;
}
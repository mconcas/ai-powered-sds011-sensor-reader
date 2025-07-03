#include "../../include/plugin_interface.h"
#include "../../include/sds011_reader.h"
#include <ncurses.h>
#include <memory>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <dirent.h>
#include <cstring>

/**
 * @brief SDS011 sensor data
 */
class SDS011Data : public SensorData {
public:
    float pm25;
    float pm10;
    std::chrono::system_clock::time_point timestamp;
    
    SDS011Data(float pm25_val, float pm10_val) 
        : pm25(pm25_val), pm10(pm10_val), timestamp(std::chrono::system_clock::now()) {}
    
    std::string getDisplayString() const override {
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        auto tm = *std::localtime(&time_t);
        
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
            << std::setw(2) << tm.tm_min << ":" << std::setw(2) << tm.tm_sec
            << "   PM2.5: " << std::setw(5) << std::fixed << std::setprecision(1) << pm25
            << "   PM10: " << std::setw(5) << pm10;
        return oss.str();
    }
    
    std::string getQualityDescription() const override {
        if (pm25 <= 15.0) return "Good";
        if (pm25 <= 25.0) return "Moderate";
        return "Poor";
    }
    
    int getColorCode() const override {
        if (pm25 <= 15.0) return 1; // Green
        if (pm25 <= 25.0) return 2; // Yellow
        return 3; // Red
    }
};

/**
 * @brief SDS011 sensor implementation
 */
class SDS011Sensor : public PluginSensor {
private:
    std::unique_ptr<SDS011Reader> reader;
    bool connected;
    
public:
    SDS011Sensor() : connected(false) {}
    
    bool initialize(const std::string& port) override {
        reader = std::make_unique<SDS011Reader>(port);
        connected = reader->initialize();
        return connected;
    }
    
    void cleanup() override {
        if (reader) {
            reader.reset();
        }
        connected = false;
    }
    
    bool isConnected() const override {
        return connected;
    }
    
    std::unique_ptr<SensorData> readData() override {
        if (!connected || !reader) {
            return nullptr;
        }
        
        float pm25, pm10;
        if (reader->readPM25Data(pm25, pm10)) {
            return std::make_unique<SDS011Data>(pm25, pm10);
        }
        return nullptr;
    }
    
    bool calibrate() override {
        // SDS011 doesn't support calibration
        return true;
    }
    
    void reset() override {
        if (reader) {
            // Implement reset if needed
        }
    }
    
    std::string getSensorName() const override {
        return "SDS011";
    }
    
    std::string getVersion() const override {
        return "1.0.0";
    }
    
    std::vector<std::string> getSupportedDevices() const override {
        return {"SDS011", "Nova PM Sensor SDS011"};
    }
};

/**
 * @brief SDS011 UI implementation
 */
class SDS011UI : public PluginUI {
private:
    WINDOW* headerWin;
    WINDOW* dataWin;
    WINDOW* statsWin;
    WINDOW* statusWin;
    int maxY, maxX;
    
    static const int HEADER_HEIGHT = 3;
    static const int STATUS_HEIGHT = 2;
    static const int STATS_HEIGHT = 4;
    
public:
    SDS011UI() : headerWin(nullptr), dataWin(nullptr), statsWin(nullptr), statusWin(nullptr) {}
    
    ~SDS011UI() {
        cleanup();
    }
    
    bool initialize(int maxY, int maxX) override {
        this->maxY = maxY;
        this->maxX = maxX;
        createWindows();
        return true;
    }
    
    void cleanup() override {
        if (headerWin) { delwin(headerWin); headerWin = nullptr; }
        if (dataWin) { delwin(dataWin); dataWin = nullptr; }
        if (statsWin) { delwin(statsWin); statsWin = nullptr; }
        if (statusWin) { delwin(statusWin); statusWin = nullptr; }
    }
    
    void createWindows() override {
        cleanup();
        
        if (maxY < 15 || maxX < 60) return; // Too small
        
        // Create windows
        headerWin = newwin(HEADER_HEIGHT, maxX, 0, 0);
        int dataHeight = maxY - HEADER_HEIGHT - STATS_HEIGHT - STATUS_HEIGHT;
        dataWin = newwin(dataHeight, maxX, HEADER_HEIGHT, 0);
        statsWin = newwin(STATS_HEIGHT, maxX, HEADER_HEIGHT + dataHeight, 0);
        statusWin = newwin(STATUS_HEIGHT, maxX, maxY - STATUS_HEIGHT, 0);
        
        if (dataWin) scrollok(dataWin, TRUE);
        
        // Draw borders
        if (headerWin) box(headerWin, 0, 0);
        if (dataWin) box(dataWin, 0, 0);
        if (statsWin) box(statsWin, 0, 0);
        if (statusWin) box(statusWin, 0, 0);
    }
    
    void resize(int newMaxY, int newMaxX) override {
        maxY = newMaxY;
        maxX = newMaxX;
        createWindows();
    }
    
    void showHeader(const std::string& port, const std::string& status) override {
        if (!headerWin) return;
        
        wclear(headerWin);
        box(headerWin, 0, 0);
        
        if (has_colors()) {
            wattron(headerWin, COLOR_PAIR(4) | A_BOLD);
        }
        mvwprintw(headerWin, 1, 2, "SDS011 PM2.5/PM10 Sensor - %s", status.c_str());
        mvwprintw(headerWin, 2, 2, "Port: %s | Controls: 'b' Back, 'c' Clear, 'q' Quit", port.c_str());
        if (has_colors()) {
            wattroff(headerWin, COLOR_PAIR(4) | A_BOLD);
        }
        
        wrefresh(headerWin);
    }
    
    void updateDataDisplay(const std::vector<std::unique_ptr<SensorData>>& readings) override {
        if (!dataWin || readings.empty()) return;
        
        wclear(dataWin);
        box(dataWin, 0, 0);
        
        // Header
        if (has_colors()) {
            wattron(dataWin, COLOR_PAIR(4) | A_BOLD);
        }
        mvwprintw(dataWin, 1, 2, "Time      PM2.5 (µg/m³)  PM10 (µg/m³)   Quality");
        mvwprintw(dataWin, 2, 2, "%s", std::string(maxX - 6, '-').c_str());
        if (has_colors()) {
            wattroff(dataWin, COLOR_PAIR(4) | A_BOLD);
        }
        
        // Display recent readings
        int line = 3;
        int maxLines = maxY - HEADER_HEIGHT - STATS_HEIGHT - STATUS_HEIGHT - 2;
        
        for (auto it = readings.rbegin(); it != readings.rend() && line < maxLines; ++it, ++line) {
            int colorPair = (*it)->getColorCode();
            std::string quality = (*it)->getQualityDescription();
            
            if (has_colors()) {
                wattron(dataWin, COLOR_PAIR(colorPair));
            }
            
            std::string displayStr = (*it)->getDisplayString() + "   " + quality;
            mvwprintw(dataWin, line, 2, "%s", displayStr.c_str());
            
            if (has_colors()) {
                wattroff(dataWin, COLOR_PAIR(colorPair));
            }
        }
        
        wrefresh(dataWin);
    }
    
    void updateStatistics(const std::vector<std::unique_ptr<SensorData>>& readings) override {
        if (!statsWin || readings.empty()) return;
        
        wclear(statsWin);
        box(statsWin, 0, 0);
        
        if (has_colors()) {
            wattron(statsWin, COLOR_PAIR(4) | A_BOLD);
        }
        mvwprintw(statsWin, 0, 2, "Statistics (last %zu readings)", readings.size());
        if (has_colors()) {
            wattroff(statsWin, COLOR_PAIR(4) | A_BOLD);
        }
        
        // Calculate statistics
        if (!readings.empty()) {
            float sumPM25 = 0, sumPM10 = 0;
            float minPM25 = 1000, maxPM25 = 0;
            float minPM10 = 1000, maxPM10 = 0;
            
            for (const auto& reading : readings) {
                const SDS011Data* data = dynamic_cast<const SDS011Data*>(reading.get());
                if (data) {
                    sumPM25 += data->pm25;
                    sumPM10 += data->pm10;
                    minPM25 = std::min(minPM25, data->pm25);
                    maxPM25 = std::max(maxPM25, data->pm25);
                    minPM10 = std::min(minPM10, data->pm10);
                    maxPM10 = std::max(maxPM10, data->pm10);
                }
            }
            
            float avgPM25 = sumPM25 / readings.size();
            float avgPM10 = sumPM10 / readings.size();
            
            mvwprintw(statsWin, 1, 2, "PM2.5: Avg %.1f  Min %.1f  Max %.1f µg/m³", avgPM25, minPM25, maxPM25);
            mvwprintw(statsWin, 2, 2, "PM10:  Avg %.1f  Min %.1f  Max %.1f µg/m³", avgPM10, minPM10, maxPM10);
        }
        
        wrefresh(statsWin);
    }
    
    void showError(const std::string& message) override {
        if (!statusWin) return;
        
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
    
    void showStatus(const std::string& status) override {
        if (!statusWin) return;
        
        wclear(statusWin);
        box(statusWin, 0, 0);
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        if (has_colors()) {
            wattron(statusWin, COLOR_PAIR(5));
        }
        mvwprintw(statusWin, 1, 2, "Status: %s | Last update: %02d:%02d:%02d",
                 status.c_str(), tm.tm_hour, tm.tm_min, tm.tm_sec);
        if (has_colors()) {
            wattroff(statusWin, COLOR_PAIR(5));
        }
        
        wrefresh(statusWin);
    }
    
    int handleInput() override {
        int ch = getch();
        switch (ch) {
            case 'q':
            case 'Q':
                return 1; // Quit
            case 'b':
            case 'B':
                return 2; // Back to menu
            case 'c':
            case 'C':
                return 3; // Clear data
            case KEY_RESIZE:
                return 4; // Resize
            default:
                return 0; // Continue
        }
    }
    
    std::string getPluginName() const override {
        return "SDS011 UI";
    }
    
    std::string getVersion() const override {
        return "1.0.0";
    }
};

/**
 * @brief SDS011 plugin implementation
 */
class SDS011Plugin : public Plugin {
public:
    bool initialize() override {
        return true;
    }
    
    void cleanup() override {
        // Nothing to cleanup
    }
    
    std::vector<DeviceInfo> detectDevices() const override {
        std::vector<DeviceInfo> devices;
        
#ifdef __APPLE__
        // Scan /dev for cu.usbserial* devices
        DIR* dir = opendir("/dev");
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string filename = entry->d_name;
                if (filename.find("cu.usbserial") == 0 || 
                    filename.find("cu.usbmodem") == 0 ||
                    filename.find("cu.SLAB_USBtoUART") == 0 ||
                    filename.find("cu.wchusbserial") == 0) {
                    
                    std::string fullPath = "/dev/" + filename;
                    
                    // Quick test to see if device exists and is accessible
                    FILE* file = fopen(fullPath.c_str(), "r");
                    if (file) {
                        fclose(file);
                        
                        DeviceInfo device;
                        device.port = fullPath;
                        device.vendor_id = "1a86"; // Common VID for SDS011
                        device.product_id = "7523"; // Common PID for SDS011
                        device.description = "SDS011 PM2.5/PM10 Sensor";
                        device.accessible = true;
                        
                        devices.push_back(device);
                    }
                }
            }
            closedir(dir);
        }
#else
        // Linux - use traditional approach
        std::vector<std::string> testPorts = {
            "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2", "/dev/ttyUSB3",
            "/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyACM2", "/dev/ttyACM3"
        };
        
        for (const auto& port : testPorts) {
            // Quick test to see if device exists and is accessible
            FILE* file = fopen(port.c_str(), "r");
            if (file) {
                fclose(file);
                
                DeviceInfo device;
                device.port = port;
                device.vendor_id = "1a86"; // Common VID for SDS011
                device.product_id = "7523"; // Common PID for SDS011
                device.description = "SDS011 PM2.5/PM10 Sensor";
                device.accessible = true;
                
                devices.push_back(device);
            }
        }
#endif
        
        return devices;
    }
    
    bool canHandleDevice(const DeviceInfo& device) const override {
        // Check if device matches SDS011 patterns
        return device.description.find("SDS011") != std::string::npos ||
               device.vendor_id == "1a86" ||
               device.port.find("ttyUSB") != std::string::npos ||
               device.port.find("cu.usbserial") != std::string::npos;
    }
    
    double getDeviceMatchScore(const DeviceInfo& device) const override {
        double score = 0.0;
        
        if (device.description.find("SDS011") != std::string::npos) score += 1.0;
        if (device.vendor_id == "1a86") score += 0.8;
        if (device.product_id == "7523") score += 0.8;
        if (device.port.find("ttyUSB") != std::string::npos) score += 0.5;
        if (device.port.find("cu.usbserial") != std::string::npos) score += 0.5;
        
        return score;
    }
    
    std::unique_ptr<PluginSensor> createSensor() override {
        return std::make_unique<SDS011Sensor>();
    }
    
    std::unique_ptr<PluginUI> createUI() override {
        return std::make_unique<SDS011UI>();
    }
    
    std::string getPluginName() const override {
        return "SDS011";
    }
    
    std::string getVersion() const override {
        return "1.0.0";
    }
    
    std::string getDescription() const override {
        return "SDS011 PM2.5/PM10 Particulate Matter Sensor Plugin";
    }
    
    std::vector<std::string> getSupportedDevicePatterns() const override {
        return {"SDS011", "Nova PM Sensor", "1a86:7523"};
    }
};

// Plugin entry points
PLUGIN_API Plugin* createPlugin() {
    return new SDS011Plugin();
}

PLUGIN_API void destroyPlugin(Plugin* plugin) {
    delete plugin;
}

PLUGIN_API const char* getPluginName() {
    return "SDS011";
}

PLUGIN_API const char* getPluginVersion() {
    return "1.0.0";
}

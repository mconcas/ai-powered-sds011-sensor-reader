#pragma once

#include <chrono>
#include <deque>
#include <string>
#include <ncurses.h>

/**
 * @brief Structure to store sensor readings with timestamp
 */
struct SensorReading {
    std::chrono::system_clock::time_point timestamp;
    float pm25;
    float pm10;
    
    SensorReading(float p25, float p10) 
        : timestamp(std::chrono::system_clock::now()), pm25(p25), pm10(p10) {}
};

/**
 * @brief Text User Interface for SDS011 sensor data display
 * 
 * This class provides an interactive ncurses-based interface for
 * displaying real-time sensor data with color-coded air quality indicators.
 */
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
    
    /**
     * @brief Create and position all windows
     */
    void createWindows();
    
    /**
     * @brief Update the data display window
     */
    void updateDataWindow();
    
    /**
     * @brief Update the statistics window
     */
    void updateStatsWindow();
    
    /**
     * @brief Update the status window
     */
    void updateStatusWindow();
    
public:
    /**
     * @brief Constructor
     */
    SDS011TUI();
    
    /**
     * @brief Destructor - cleans up ncurses resources
     */
    ~SDS011TUI();
    
    /**
     * @brief Initialize the TUI interface
     * @return true if initialization successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Clean up ncurses resources
     */
    void cleanup();
    
    /**
     * @brief Draw the header window
     * @param port The serial port being used
     */
    void drawHeader(const std::string& port);
    
    /**
     * @brief Add a new sensor reading to the display
     * @param pm25 PM2.5 value in µg/m³
     * @param pm10 PM10 value in µg/m³
     */
    void addReading(float pm25, float pm10);
    
    /**
     * @brief Display an error message
     * @param message The error message to display
     */
    void showError(const std::string& message);
    
    /**
     * @brief Clear all collected data
     */
    void clearData();
    
    /**
     * @brief Handle user input
     * @return 1 if user wants to quit, 0 otherwise
     */
    int handleInput();
};

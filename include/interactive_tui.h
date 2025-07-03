#pragma once

#include "sensor_plugin.h"
#include "sensor_registry.h"
#include <ncurses.h>
#include <deque>
#include <memory>

/**
 * @brief Interactive TUI for sensor selection and monitoring
 */
class InteractiveTUI {
private:
    WINDOW* mainWin;
    WINDOW* headerWin;
    WINDOW* menuWin;
    WINDOW* dataWin;
    WINDOW* statsWin;
    WINDOW* statusWin;
    
    SensorRegistry registry;
    std::unique_ptr<SensorPlugin> currentSensor;
    std::deque<std::unique_ptr<SensorData>> readings;
    static const size_t MAX_READINGS = 100;
    
    int maxY, maxX;
    bool inSensorMode;
    
    /**
     * @brief Get navigation symbols (Unicode or ASCII fallback)
     * @return String with navigation symbols
     */
    std::string getNavigationSymbols() const;
    
    /**
     * @brief Create and position all windows
     */
    void createWindows();
    
    /**
     * @brief Show sensor selection menu
     */
    void showSensorMenu();
    
    /**
     * @brief Show sensor data display
     */
    void showSensorData();
    
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
    
    /**
     * @brief Handle input in menu mode
     */
    int handleMenuInput();
    
    /**
     * @brief Handle input in sensor mode
     */
    int handleSensorInput();
    
    /**
     * @brief Select and initialize a sensor
     */
    bool selectSensor(const SensorInfo& info);
    
public:
    /**
     * @brief Constructor
     */
    InteractiveTUI();
    
    /**
     * @brief Destructor
     */
    ~InteractiveTUI();
    
    /**
     * @brief Initialize the TUI
     */
    bool initialize();
    
    /**
     * @brief Main event loop
     */
    void run();
    
    /**
     * @brief Clean up resources
     */
    void cleanup();
    
    /**
     * @brief Add a new sensor reading
     */
    void addReading(std::unique_ptr<SensorData> data);
    
    /**
     * @brief Show error message
     */
    void showError(const std::string& message);
    
    /**
     * @brief Clear all collected data
     */
    void clearData();
};

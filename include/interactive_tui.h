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
    WINDOW *mainWin;
    WINDOW *headerWin;
    WINDOW *menuWin;
    WINDOW *dataWin;
    WINDOW *statsWin;
    WINDOW *statusWin;

    // Window layout constants
    static const int HEADER_HEIGHT = 3;
    static const int STATUS_HEIGHT = 2;
    static const int STATS_HEIGHT = 3;
    static const int MIN_TERMINAL_HEIGHT = 20;
    static const int MIN_TERMINAL_WIDTH = 80;

    int selectedIndex;
    int maxX, maxY;

    // Application state
    bool inSensorMode;
    bool devicesScanned;
    std::unique_ptr<SensorPlugin> currentSensor;
    std::vector<SensorInfo> cachedSensors;
    std::vector<SensorInfo> cachedAllDevices;

    // Sensor data
    std::deque<std::unique_ptr<SensorData>> readings;
    static const size_t MAX_READINGS = 100; // Max readings to store

    SensorRegistry registry;

    void createWindows();
    void cleanupWindows();
    void performDeviceScan();
    void refreshDevices();

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
    bool selectSensor(const SensorInfo &info);

    /**
     * @brief Get a list of available sensors
     */
    std::vector<SensorInfo> getAvailableSensors() const;

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
    void showError(const std::string &message);

    /**
     * @brief Clear all collected data
     */
    void clearData();
};

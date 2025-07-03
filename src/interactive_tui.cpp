#include "interactive_tui.h"
#include "plugin_manager.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <thread>

InteractiveTUI::InteractiveTUI() 
    : mainWin(nullptr), headerWin(nullptr), menuWin(nullptr), 
      dataWin(nullptr), statsWin(nullptr), statusWin(nullptr),
      selectedIndex(0), inSensorMode(false), devicesScanned(false), 
      currentSensor(nullptr), currentUI(nullptr), currentPlugin(nullptr) {
    
    // Initialize plugin manager
    pluginManager.reset(new PluginManager("./build/plugins"));
}

InteractiveTUI::~InteractiveTUI() {
    cleanup();
}

bool InteractiveTUI::initialize() {
    // Initialize ncurses
    mainWin = initscr();
    if (mainWin == nullptr) {
        return false;
    }
    
    // Configure ncurses
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);
    
    // Initialize colors
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);   // Good values
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // Moderate values
        init_pair(3, COLOR_RED, COLOR_BLACK);     // High values
        init_pair(4, COLOR_CYAN, COLOR_BLACK);    // Headers
        init_pair(5, COLOR_WHITE, COLOR_BLUE);    // Status bar
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK); // Menu items
    }
    
    getmaxyx(stdscr, maxY, maxX);
    createWindows();
    
    return true;
}

void InteractiveTUI::cleanupWindows() {
    if (headerWin) { delwin(headerWin); headerWin = nullptr; }
    if (menuWin) { delwin(menuWin); menuWin = nullptr; }
    if (dataWin) { delwin(dataWin); dataWin = nullptr; }
    if (statsWin) { delwin(statsWin); statsWin = nullptr; }
    if (statusWin) { delwin(statusWin); statusWin = nullptr; }
}

void InteractiveTUI::createWindows() {
    // Ensure we have valid terminal dimensions
    getmaxyx(stdscr, maxY, maxX);
    if (maxY < MIN_TERMINAL_HEIGHT || maxX < MIN_TERMINAL_WIDTH) {
        return; // Terminal too small, don't create windows
    }
    
    // Clean up existing windows first
    cleanupWindows();
    
    // Clear screen before creating new windows
    clear();
    refresh();
    
    // Header window (top 3 lines)
    headerWin = newwin(HEADER_HEIGHT, maxX, 0, 0);
    if (!headerWin) return; // Failed to create header window
    
    if (inSensorMode) {
        // Sensor monitoring layout
        int dataWinHeight = maxY - HEADER_HEIGHT - STATS_HEIGHT - STATUS_HEIGHT;
        dataWin = newwin(dataWinHeight, maxX, HEADER_HEIGHT, 0);
        statsWin = newwin(STATS_HEIGHT, maxX, maxY - STATUS_HEIGHT - STATS_HEIGHT, 0);
        statusWin = newwin(STATUS_HEIGHT, maxX, maxY - STATUS_HEIGHT, 0);
        
        // Check if all windows were created successfully
        if (!dataWin || !statsWin || !statusWin) {
            cleanupWindows();
            return;
        }
        
        if (dataWin) scrollok(dataWin, TRUE);
        
    } else {
        // Menu layout
        int menuWinHeight = maxY - HEADER_HEIGHT - STATUS_HEIGHT;
        menuWin = newwin(menuWinHeight, maxX, HEADER_HEIGHT, 0);
        statusWin = newwin(STATUS_HEIGHT, maxX, maxY - STATUS_HEIGHT, 0);
        
        // Check if windows were created successfully
        if (!menuWin || !statusWin) {
            cleanupWindows();
            return;
        }
    }
    
    // Draw boxes around all windows
    if (headerWin) box(headerWin, 0, 0);
    if (menuWin) box(menuWin, 0, 0);
    if (dataWin) box(dataWin, 0, 0);
    if (statsWin) box(statsWin, 0, 0);
    if (statusWin) box(statusWin, 0, 0);
}

void InteractiveTUI::performDeviceScan() {
    // Show scanning status
    if (statusWin) {
        wclear(statusWin);
        box(statusWin, 0, 0);
        mvwprintw(statusWin, 1, 2, "Loading plugins and scanning devices... Please wait.");
        wrefresh(statusWin);
    }
    
    // Load all available plugins
    if (!pluginManager->loadAllPlugins()) {
        showError("Warning: Some plugins failed to load");
    }
    
    // Perform device discovery using plugins
    cachedDevices = pluginManager->detectAllDevices();
    devicesScanned = true;
    
    // Clear the scanning status
    if (statusWin) {
        wclear(statusWin);
        box(statusWin, 0, 0);
        wrefresh(statusWin);
    }
}

void InteractiveTUI::refreshDevices() {
    // Clear cached data and rescan
    cachedDevices.clear();
    devicesScanned = false;
    selectedIndex = 0;  // Reset selection
    performDeviceScan();
}

void InteractiveTUI::run() {
    // Perform initial device scan if not already done
    if (!devicesScanned) {
        performDeviceScan();
    }
    
    while (true) {
        if (inSensorMode && currentSensor) {
            showSensorData();
            if (handleSensorInput() == 1) {
                break;
            }
            
            // Try to read sensor data
            auto data = currentSensor->readData();
            if (data) {
                addReading(std::move(data));
            }
            
        } else {
            showSensorMenu();
            if (handleMenuInput() == 1) {
                break;
            }
        }
        
        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void InteractiveTUI::showSensorMenu() {
    // Draw header
    if (headerWin) {
        wclear(headerWin);
        box(headerWin, 0, 0);
        
        if (has_colors()) {
            wattron(headerWin, COLOR_PAIR(4) | A_BOLD);
        }
        mvwprintw(headerWin, 1, 2, "Interactive Sensor Monitor - Sensor Selection");
        mvwprintw(headerWin, 2, 2, "Use arrow keys (^v) to select, Enter to connect, 'q' to quit, 'r' to refresh");
        if (has_colors()) {
            wattroff(headerWin, COLOR_PAIR(4) | A_BOLD);
        }
        wrefresh(headerWin);
    }
    
    // Draw menu
    wclear(menuWin);
    box(menuWin, 0, 0);
    
    if (has_colors()) {
        wattron(menuWin, COLOR_PAIR(4) | A_BOLD);
    }
    mvwprintw(menuWin, 1, 2, "Available Sensors:");
    mvwprintw(menuWin, 2, 2, "%-15s %-10s %-40s %s", "Port", "Type", "Description", "Status");
    mvwprintw(menuWin, 3, 2, "%s", std::string(maxX - 6, '-').c_str());
    if (has_colors()) {
        wattroff(menuWin, COLOR_PAIR(4) | A_BOLD);
    }
    
    int line = 4;
    
    // Use cached sensor data instead of rescanning
    const auto& availableDevices = getAvailableDevices();
    
    if (availableDevices.empty()) {
        if (has_colors()) {
            wattron(menuWin, COLOR_PAIR(3));
        }
        mvwprintw(menuWin, line++, 2, "No compatible devices detected.");
        mvwprintw(menuWin, line++, 2, "");
        if (has_colors()) {
            wattroff(menuWin, COLOR_PAIR(3));
        }
        
        if (has_colors()) {
            wattron(menuWin, COLOR_PAIR(4));
        }
        mvwprintw(menuWin, line++, 2, "Found %zu device(s) scanned by plugins:", cachedDevices.size());
        if (has_colors()) {
            wattroff(menuWin, COLOR_PAIR(4));
        }
        
        // Use cached device information when no devices are accessible
        const auto& allDevices = cachedDevices;
        if (!allDevices.empty()) {
            mvwprintw(menuWin, line++, 2, "");
            if (has_colors()) {
                wattron(menuWin, COLOR_PAIR(4) | A_BOLD);
            }
            mvwprintw(menuWin, line++, 2, "%-20s %-15s %-10s %-20s %s", 
                     "Port", "Description", "VID:PID", "Plugin Support", "Status");
            mvwprintw(menuWin, line++, 2, "%s", std::string(maxX - 6, '-').c_str());
            if (has_colors()) {
                wattroff(menuWin, COLOR_PAIR(4) | A_BOLD);
            }
            
            for (const auto& device : allDevices) {
                if (line >= maxY - 4) break; // Prevent overflow
                
                std::string vid_pid = device.vendor_id + ":" + device.product_id;
                Plugin* plugin = pluginManager->findBestPluginForDevice(device);
                std::string plugin_support = plugin ? plugin->getPluginName() : "None";
                std::string status = device.accessible ? "Accessible" : "Blocked";
                
                // Color code based on accessibility and plugin support
                if (has_colors()) {
                    if (device.accessible && plugin) {
                        wattron(menuWin, COLOR_PAIR(1)); // Green for working devices
                    } else if (plugin) {
                        wattron(menuWin, COLOR_PAIR(2)); // Yellow for supported but inaccessible
                    } else {
                        wattron(menuWin, COLOR_PAIR(3)); // Red for unsupported
                    }
                }
                
                mvwprintw(menuWin, line++, 2, "%-20s %-15s %-10s %-20s %s", 
                         device.port.c_str(),
                         device.description.c_str(),
                         vid_pid.c_str(),
                         plugin_support.c_str(),
                         status.c_str());
                
                if (has_colors()) {
                    wattroff(menuWin, COLOR_PAIR(6));
                    wattroff(menuWin, COLOR_PAIR(3));
                    wattroff(menuWin, COLOR_PAIR(2));
                    wattroff(menuWin, COLOR_PAIR(1));
                }
            }
            
            // Show permission fix suggestions
            if (line < maxY - 8) {
                mvwprintw(menuWin, line++, 2, "");
                if (has_colors()) {
                    wattron(menuWin, COLOR_PAIR(4));
                }
                mvwprintw(menuWin, line++, 2, "To fix permission issues:");
#ifdef MACOS
                mvwprintw(menuWin, line++, 2, "  sudo chmod 666 /dev/tty.* /dev/cu.*");
#else
                mvwprintw(menuWin, line++, 2, "  sudo chmod 666 /dev/ttyUSB* /dev/ttyACM*");
#endif
                mvwprintw(menuWin, line++, 2, "  sudo usermod -a -G dialout $USER");
                mvwprintw(menuWin, line++, 2, "  (then logout and login again)");
                if (has_colors()) {
                    wattroff(menuWin, COLOR_PAIR(4));
                }
            }
        } else {
            mvwprintw(menuWin, line++, 2, "");
            if (has_colors()) {
                wattron(menuWin, COLOR_PAIR(3));
            }
            mvwprintw(menuWin, line++, 2, "No serial devices found in /dev.");
            mvwprintw(menuWin, line++, 2, "Please check that your sensor is connected.");
            if (has_colors()) {
                wattroff(menuWin, COLOR_PAIR(3));
            }
        }
    } else {
        // Ensure selected index is valid
        selectedIndex = std::max(0, std::min(selectedIndex, (int)availableDevices.size() - 1));
        
        for (size_t i = 0; i < availableDevices.size() && line < maxY - 7; ++i, ++line) {
            const auto& device = availableDevices[i];
            Plugin* plugin = pluginManager->findBestPluginForDevice(device);
            std::string pluginName = plugin ? plugin->getPluginName() : "Unknown";
            
            if ((int)i == selectedIndex) {
                if (has_colors()) {
                    wattron(menuWin, COLOR_PAIR(6) | A_REVERSE);
                }
                mvwprintw(menuWin, line, 2, "> %-13s %-10s %-38s Ready", 
                         device.port.c_str(), pluginName.c_str(), device.description.c_str());
                if (has_colors()) {
                    wattroff(menuWin, COLOR_PAIR(6) | A_REVERSE);
                }
            } else {
                mvwprintw(menuWin, line, 2, "  %-13s %-10s %-38s Available", 
                         device.port.c_str(), pluginName.c_str(), device.description.c_str());
            }
        }
    }
    
    wrefresh(menuWin);
    
    // Update status
    wclear(statusWin);
    box(statusWin, 0, 0);
    
    std::ostringstream status;
    if (availableDevices.empty()) {
        status << "No working sensors found | "
               << "Controls: R Refresh, Q Quit";
    } else {
        status << "Found " << availableDevices.size() << " available device(s) | "
               << "Controls: ^v Navigate, Enter Select, R Refresh, Q Quit";
    }
    
    mvwprintw(statusWin, 1, 2, "%s", status.str().c_str());
    wrefresh(statusWin);
}

void InteractiveTUI::showSensorData() {
    if (!currentSensor) return;
    
    // Update header with plugin title
    updateSensorHeader();
    
    // Convert deque to vector for plugin interface
    std::vector<std::unique_ptr<SensorData>> vectorReadings;
    // Note: Currently using enhanced display methods directly
    // For future compatibility with plugin UI system
    
    // Use our enhanced display methods instead of plugin UI for now
    updateDataWindow();
    updateStatsWindow();
    updateStatusWindow();
}

void InteractiveTUI::updateDataWindow() {
    if (!dataWin) return;
    
    wclear(dataWin);
    box(dataWin, 0, 0);
    
    // Header
    if (has_colors()) {
        wattron(dataWin, COLOR_PAIR(4) | A_BOLD);
    }
    mvwprintw(dataWin, 1, 2, "Sensor Data Display");
    mvwprintw(dataWin, 2, 2, "%s", std::string(maxX - 6, '-').c_str());
    if (has_colors()) {
        wattroff(dataWin, COLOR_PAIR(4) | A_BOLD);
    }
    
    if (readings.empty()) {
        mvwprintw(dataWin, 4, 2, "No data collected yet...");
    } else {
        mvwprintw(dataWin, 4, 2, "Total readings: %zu", readings.size());
        
        // Show latest reading with colored values
        if (!readings.empty()) {
            const auto& latest = readings.back();
            int colorCode = latest->getColorCode();
            std::string quality = latest->getQualityDescription();
            
            // Show latest reading with color coding
            if (has_colors()) {
                wattron(dataWin, COLOR_PAIR(colorCode) | A_BOLD);
            }
            mvwprintw(dataWin, 5, 2, "Latest: %s", latest->getDisplayString().c_str());
            if (has_colors()) {
                wattroff(dataWin, COLOR_PAIR(colorCode) | A_BOLD);
            }
            
            // Show quality description with appropriate color
            if (has_colors()) {
                wattron(dataWin, COLOR_PAIR(colorCode));
            }
            mvwprintw(dataWin, 6, 2, "Air Quality: %s", quality.c_str());
            if (has_colors()) {
                wattroff(dataWin, COLOR_PAIR(colorCode));
            }
            
            // Show recent readings history with colors
            if (readings.size() > 1) {
                mvwprintw(dataWin, 8, 2, "Recent readings:");
                int line = 9;
                int maxLines = std::min(15, (int)readings.size()); // Show last 15 readings
                
                for (int i = std::max(0, (int)readings.size() - maxLines); 
                     i < (int)readings.size() && line < maxY - HEADER_HEIGHT - STATS_HEIGHT - STATUS_HEIGHT - 2; 
                     ++i, ++line) {
                    
                    const auto& reading = readings[i];
                    int readingColor = reading->getColorCode();
                    
                    if (has_colors()) {
                        wattron(dataWin, COLOR_PAIR(readingColor));
                    }
                    mvwprintw(dataWin, line, 4, "%s  %s", 
                             reading->getDisplayString().c_str(),
                             reading->getQualityDescription().c_str());
                    if (has_colors()) {
                        wattroff(dataWin, COLOR_PAIR(readingColor));
                    }
                }
            }
        }
    }
    
    wrefresh(dataWin);
}

void InteractiveTUI::updateStatsWindow() {
    if (!statsWin) return;
    
    wclear(statsWin);
    box(statsWin, 0, 0);
    
    if (has_colors()) {
        wattron(statsWin, COLOR_PAIR(4) | A_BOLD);
    }
    mvwprintw(statsWin, 0, 2, "Statistics (last %zu readings)", readings.size());
    if (has_colors()) {
        wattroff(statsWin, COLOR_PAIR(4) | A_BOLD);
    }
    
    if (!readings.empty()) {
        // Calculate basic statistics
        size_t readingCount = readings.size();
        
        // For detailed statistics, let the plugin handle it if available
        if (currentUI && false) { // Disable plugin UI for now due to interface mismatch
            // Plugin will handle detailed stats
            // currentUI->updateStatistics(readings);
        } else {
            // Basic fallback display
            mvwprintw(statsWin, 1, 2, "Total readings: %zu", readingCount);
            
            // Show latest with color
            const auto& latest = readings.back();
            int colorCode = latest->getColorCode();
            
            if (has_colors()) {
                wattron(statsWin, COLOR_PAIR(colorCode));
            }
            mvwprintw(statsWin, 2, 2, "Latest data: %s", latest->getDisplayString().c_str());
            if (has_colors()) {
                wattroff(statsWin, COLOR_PAIR(colorCode));
            }
            
            // Show color legend
            mvwprintw(statsWin, 1, maxX/2, "Legend: ");
            if (has_colors()) {
                wattron(statsWin, COLOR_PAIR(1));
                mvwprintw(statsWin, 1, maxX/2 + 8, "Good");
                wattroff(statsWin, COLOR_PAIR(1));
                
                wattron(statsWin, COLOR_PAIR(2));
                mvwprintw(statsWin, 1, maxX/2 + 14, "Moderate");
                wattroff(statsWin, COLOR_PAIR(2));
                
                wattron(statsWin, COLOR_PAIR(3));
                mvwprintw(statsWin, 1, maxX/2 + 24, "Poor");
                wattroff(statsWin, COLOR_PAIR(3));
            } else {
                mvwprintw(statsWin, 1, maxX/2 + 8, "Good  Moderate  Poor");
            }
        }
    } else {
        mvwprintw(statsWin, 1, 2, "No data available");
    }
    
    wrefresh(statsWin);
}

void InteractiveTUI::updateStatusWindow() {
    if (!statusWin) return;
    
    wclear(statusWin);
    box(statusWin, 0, 0);
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    if (has_colors()) {
        wattron(statusWin, COLOR_PAIR(5));
    }
    
    mvwprintw(statusWin, 1, 2, "Status: Active | Last update: %02d:%02d:%02d | Total readings: %zu",
             tm.tm_hour, tm.tm_min, tm.tm_sec, readings.size());
    
    if (has_colors()) {
        wattroff(statusWin, COLOR_PAIR(5));
    }
    
    wrefresh(statusWin);
}

int InteractiveTUI::handleMenuInput() {
    const auto availableDevices = getAvailableDevices();

    int ch = getch();
    switch (ch) {
        case 'q':
        case 'Q':
            return 1; // Quit

        case KEY_UP:
            if (!availableDevices.empty()) {
                selectedIndex = std::max(0, selectedIndex - 1);
            }
            break;

        case KEY_DOWN:
            if (!availableDevices.empty()) {
                selectedIndex = std::min(selectedIndex + 1, (int)availableDevices.size() - 1);
            }
            break;

        case '\n':
        case '\r':
        case KEY_ENTER:
            if (selectedIndex >= 0 && selectedIndex < (int)availableDevices.size()) {
                if (selectSensor(availableDevices[selectedIndex])) {
                    inSensorMode = true;
                    cleanupWindows();
                    createWindows();
                }
            }
            break;

        case 'r':
        case 'R':
            refreshDevices();
            break;

        case KEY_RESIZE:
            getmaxyx(stdscr, maxY, maxX);
            createWindows();
            break;

        case ERR:
            // Timeout - no input
            break;

        default:
            // Ignore other keys
            break;
    }
    return 0;
}

int InteractiveTUI::handleSensorInput() {
    // Always handle basic TUI commands first
    int ch = getch();
    switch (ch) {
        case 'q':
        case 'Q':
            return 1; // Quit
            
        case 'c':
        case 'C':
            clearData();
            return 0; // Continue
            
        case KEY_RESIZE:
            getmaxyx(stdscr, maxY, maxX);
            createWindows();
            if (currentUI) {
                currentUI->resize(maxY, maxX);
            }
            return 0; // Continue
            
        case ERR:
            // Timeout - no input
            return 0; // Continue
            
        default:
            // Let plugin UI handle other keys if available
            if (currentUI) {
                // Put the character back and let plugin UI handle it
                ungetch(ch);
                int result = currentUI->handleInput();
                if (result == 2) {
                    // Plugin UI wants to go back to menu
                    inSensorMode = false;
                    if (currentSensor) {
                        currentSensor->cleanup();
                        currentSensor.reset();
                    }
                    if (currentUI) {
                        currentUI->cleanup();
                        currentUI.reset();
                    }
                    currentPlugin = nullptr;
                    createWindows(); // Re-create windows for menu mode
                    return 0;
                }
                return result; // 0=continue, 1=quit
            }
            // Ignore unknown keys if no plugin UI
            return 0; // Continue
    }
}

bool InteractiveTUI::selectSensor(const DeviceInfo& device) {
    // Find the best plugin for this device
    currentPlugin = pluginManager->findBestPluginForDevice(device);
    if (!currentPlugin) {
        showError("No compatible plugin found for device: " + device.description);
        return false;
    }
    
    // Create sensor and UI instances
    currentSensor = currentPlugin->createSensor();
    currentUI = currentPlugin->createUI();
    
    if (!currentSensor) {
        showError("Failed to create sensor component");
        currentSensor.reset();
        currentUI.reset();
        currentPlugin = nullptr;
        return false;
    }
    
    // Initialize the sensor
    if (!currentSensor->initialize(device.port)) {
        std::string error_msg = "Failed to initialize sensor at " + device.port;
        if (!device.accessible) {
            error_msg += ". Device not accessible - check permissions";
        }
        showError(error_msg);
        currentSensor.reset();
        currentUI.reset();
        currentPlugin = nullptr;
        return false;
    }
    
    // Initialize the plugin UI if available (optional)
    if (currentUI && !currentUI->initialize(maxY, maxX)) {
        // Plugin UI initialization failed, but we can continue with built-in TUI
        showError("Plugin UI failed to initialize, using built-in display");
        currentUI.reset(); // Clear the failed UI, but keep the sensor
    }
    
    clearData();
    return true;
}

void InteractiveTUI::addReading(std::unique_ptr<SensorData> data) {
    readings.push_back(std::move(data));
    
    // Keep only the last MAX_READINGS
    if (readings.size() > MAX_READINGS) {
        readings.pop_front();
    }
}

void InteractiveTUI::showError(const std::string& message) {
    if (statusWin) {
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
}

void InteractiveTUI::clearData() {
    readings.clear();
}

void InteractiveTUI::cleanup() {
    cleanupWindows();
    
    if (currentSensor) {
        currentSensor->cleanup();
        currentSensor.reset();
    }
    
    if (mainWin) {
        endwin();
        mainWin = nullptr;
    }
}

std::vector<DeviceInfo> InteractiveTUI::getAvailableDevices() const {
    std::vector<DeviceInfo> available;
    for (const auto& device : cachedDevices) {
        if (device.accessible) {
            available.push_back(device);
        }
    }
    return available;
}

void InteractiveTUI::updateSensorHeader() {
    if (!headerWin || !currentPlugin) return;
    
    wclear(headerWin);
    box(headerWin, 0, 0);
    
    // Get plugin information
    std::string pluginName = currentPlugin->getPluginName();
    std::string pluginDescription = currentPlugin->getDescription();
    
    // Determine status based on recent readings
    std::string status = "Active";
    int statusColor = 1; // Green by default
    
    if (!readings.empty()) {
        // Use the color code from the latest reading for status color
        int latestColorCode = readings.back()->getColorCode();
        std::string quality = readings.back()->getQualityDescription();
        
        status = "Active - " + quality;
        statusColor = latestColorCode;
    } else {
        status = "Waiting for data...";
        statusColor = 2; // Yellow
    }
    
    // Display plugin title with colored status
    if (has_colors()) {
        wattron(headerWin, COLOR_PAIR(4) | A_BOLD);
    }
    mvwprintw(headerWin, 1, 2, "%s", pluginDescription.c_str());
    if (has_colors()) {
        wattroff(headerWin, COLOR_PAIR(4) | A_BOLD);
    }
    
    // Show colored status
    if (has_colors()) {
        wattron(headerWin, COLOR_PAIR(statusColor) | A_BOLD);
    }
    mvwprintw(headerWin, 1, maxX - status.length() - 4, "[%s]", status.c_str());
    if (has_colors()) {
        wattroff(headerWin, COLOR_PAIR(statusColor) | A_BOLD);
    }
    
    // Show controls
    if (has_colors()) {
        wattron(headerWin, COLOR_PAIR(4));
    }
    mvwprintw(headerWin, 2, 2, "Sensor Mode Controls: 'c' Clear data, 'q' Quit");
    if (has_colors()) {
        wattroff(headerWin, COLOR_PAIR(4));
    }
    
    wrefresh(headerWin);
}

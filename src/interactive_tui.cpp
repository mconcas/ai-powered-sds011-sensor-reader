#include "interactive_tui.h"
#include "sds011_plugin.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <thread>

InteractiveTUI::InteractiveTUI() 
    : mainWin(nullptr), headerWin(nullptr), menuWin(nullptr), 
      dataWin(nullptr), statsWin(nullptr), statusWin(nullptr),
      selectedIndex(0), inSensorMode(false), devicesScanned(false), currentSensor(nullptr) {
    
    // Register available sensor plugins
    registry.registerPlugin(std::unique_ptr<SensorPlugin>(new SDS011Plugin()));
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
        mvwprintw(statusWin, 1, 2, "Scanning for devices... Please wait.");
        wrefresh(statusWin);
    }
    
    // Perform initial device discovery
    cachedSensors = registry.discoverSensors();
    cachedAllDevices = registry.discoverAllDevices();
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
    cachedSensors.clear();
    cachedAllDevices.clear();
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
    const auto& availableSensors = getAvailableSensors();
    
    if (availableSensors.empty()) {
        if (has_colors()) {
            wattron(menuWin, COLOR_PAIR(3));
        }
        mvwprintw(menuWin, line++, 2, "No working sensors detected.");
        mvwprintw(menuWin, line++, 2, "");
        if (has_colors()) {
            wattroff(menuWin, COLOR_PAIR(3));
        }
        
        if (has_colors()) {
            wattron(menuWin, COLOR_PAIR(4));
        }
        mvwprintw(menuWin, line++, 2, "Found %zu serial device(s) with permission details:", cachedAllDevices.size());
        if (has_colors()) {
            wattroff(menuWin, COLOR_PAIR(4));
        }
        
        // Use cached device information when no sensors work
        const auto& allDevices = cachedAllDevices;
        if (!allDevices.empty()) {
            mvwprintw(menuWin, line++, 2, "");
            if (has_colors()) {
                wattron(menuWin, COLOR_PAIR(4) | A_BOLD);
            }
            mvwprintw(menuWin, line++, 2, "%-20s %-10s %-12s %-12s %-10s %s", 
                     "Port", "Type", "Permissions", "Owner:Group", "Access", "Issue");
            mvwprintw(menuWin, line++, 2, "%s", std::string(maxX - 6, '-').c_str());
            if (has_colors()) {
                wattroff(menuWin, COLOR_PAIR(4) | A_BOLD);
            }
            
            for (const auto& device : allDevices) {
                if (line >= maxY - 4) break; // Prevent overflow
                
                std::string owner_group = device.device_perms.owner + ":" + device.device_perms.group;
                std::string access_status = device.device_perms.getStatusString();
                std::string issue = device.device_perms.error_message.empty() ? 
                                  "OK" : device.device_perms.error_message;
                
                // Truncate long error messages for display
                if (issue.length() > 20) {
                    issue = issue.substr(0, 17) + "...";
                }
                
                // Color code based on device type and access level
                if (has_colors()) {
                    if (device.type == "SDS011" && device.available) {
                        wattron(menuWin, COLOR_PAIR(1)); // Green for working SDS011
                    } else if (device.type == "SDS011") {
                        wattron(menuWin, COLOR_PAIR(2)); // Yellow for SDS011 with issues
                    } else if (device.type == "Unsupported") {
                        wattron(menuWin, COLOR_PAIR(6)); // Magenta for unsupported
                    } else {
                        wattron(menuWin, COLOR_PAIR(3)); // Red for other issues
                    }
                }
                
                mvwprintw(menuWin, line++, 2, "%-20s %-10s %-12s %-12s %-10s %s", 
                         device.port.c_str(),
                         device.type.c_str(),
                         device.device_perms.getPermissionString().c_str(),
                         owner_group.c_str(),
                         access_status.c_str(),
                         issue.c_str());
                
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
        selectedIndex = std::max(0, std::min(selectedIndex, (int)availableSensors.size() - 1));
        
        for (size_t i = 0; i < availableSensors.size() && line < maxY - 7; ++i, ++line) {
            const auto& sensor = availableSensors[i];
            
            if ((int)i == selectedIndex) {
                if (has_colors()) {
                    wattron(menuWin, COLOR_PAIR(6) | A_REVERSE);
                }
                mvwprintw(menuWin, line, 2, "> %-13s %-10s %-38s Connected", 
                         sensor.port.c_str(), sensor.type.c_str(), sensor.description.c_str());
                if (has_colors()) {
                    wattroff(menuWin, COLOR_PAIR(6) | A_REVERSE);
                }
            } else {
                mvwprintw(menuWin, line, 2, "  %-13s %-10s %-38s Available", 
                         sensor.port.c_str(), sensor.type.c_str(), sensor.description.c_str());
            }
        }
    }
    
    wrefresh(menuWin);
    
    // Update status
    wclear(statusWin);
    box(statusWin, 0, 0);
    
    std::ostringstream status;
    if (availableSensors.empty()) {
        status << "No working sensors found | "
               << "Controls: R Refresh, Q Quit";
    } else {
        status << "Found " << availableSensors.size() << " available sensor(s) | "
               << "Controls: ^v Navigate, Enter Select, R Refresh, Q Quit";
    }
    
    mvwprintw(statusWin, 1, 2, "%s", status.str().c_str());
    wrefresh(statusWin);
}

void InteractiveTUI::showSensorData() {
    if (!currentSensor) return;
    
    // Draw header
    if (headerWin) {
        wclear(headerWin);
        box(headerWin, 0, 0);
        
        if (has_colors()) {
            wattron(headerWin, COLOR_PAIR(4) | A_BOLD);
        }
        mvwprintw(headerWin, 1, 2, "%s - %s", 
                 currentSensor->getTypeName().c_str(), 
                 currentSensor->getDescription().c_str());
        mvwprintw(headerWin, 2, 2, "Port: %s | Press 'b' to go back, 'c' to clear, 'q' to quit", 
                 currentSensor->getCurrentPort().c_str());
        if (has_colors()) {
            wattroff(headerWin, COLOR_PAIR(4) | A_BOLD);
        }
        wrefresh(headerWin);
    }
    
    updateDataWindow();
    updateStatsWindow();
    updateStatusWindow();
}

void InteractiveTUI::updateDataWindow() {
    if (!dataWin || readings.empty()) return;
    
    wclear(dataWin);
    box(dataWin, 0, 0);
    
    auto headers = currentSensor->getDisplayHeaders();
    
    // Draw headers
    if (has_colors()) {
        wattron(dataWin, COLOR_PAIR(4) | A_BOLD);
    }
    
    std::ostringstream headerLine;
    headerLine << std::left << std::setw(10) << headers[0];
    for (size_t i = 1; i < headers.size(); ++i) {
        headerLine << " " << std::setw(12) << headers[i];
    }
    
    mvwprintw(dataWin, 1, 2, "%s", headerLine.str().c_str());
    mvwprintw(dataWin, 2, 2, "%s", std::string(maxX - 6, '-').c_str());
    
    if (has_colors()) {
        wattroff(dataWin, COLOR_PAIR(4) | A_BOLD);
    }
    
    // Display readings (most recent first)
    int line = 3;
    int maxLines = maxY - 11;
    
    for (auto it = readings.rbegin(); it != readings.rend() && line < maxLines; ++it, ++line) {
        int colorPair = currentSensor->getColorCode(**it);
        std::string quality = currentSensor->getQualityDescription(**it);
        
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

void InteractiveTUI::updateStatsWindow() {
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
    
    // Calculate statistics (for SDS011 data)
    if (!readings.empty()) {
        const SDS011Data* first = dynamic_cast<const SDS011Data*>(readings[0].get());
        if (first) {
            float sumPM25 = 0, sumPM10 = 0;
            float minPM25 = first->pm25, maxPM25 = first->pm25;
            float minPM10 = first->pm10, maxPM10 = first->pm10;
            
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
            
            mvwprintw(statsWin, 1, 2, "PM2.5: Avg %.1f Min %.1f Max %.1f", avgPM25, minPM25, maxPM25);
            mvwprintw(statsWin, 2, 2, "PM10:  Avg %.1f Min %.1f Max %.1f", avgPM10, minPM10, maxPM10);
        }
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
    const auto availableSensors = getAvailableSensors();

    int ch = getch();
    switch (ch) {
        case 'q':
        case 'Q':
            return 1; // Quit

        case KEY_UP:
            if (!availableSensors.empty()) {
                selectedIndex = std::max(0, selectedIndex - 1);
            }
            break;

        case KEY_DOWN:
            if (!availableSensors.empty()) {
                selectedIndex = std::min(selectedIndex + 1, (int)availableSensors.size() - 1);
            }
            break;

        case '\n':
        case '\r':
        case KEY_ENTER:
            if (selectedIndex >= 0 && selectedIndex < (int)availableSensors.size()) {
                if (selectSensor(availableSensors[selectedIndex])) {
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
            getmaxyx(stdscr, maxY, maxX);
            createWindows();
            break;

        case 'b':
        case 'B':
            inSensorMode = false;
            if (currentSensor) {
                currentSensor->cleanup();
                currentSensor.reset();
            }
            createWindows(); // Re-create windows for menu mode
            break;
    }
    return 0;
}

bool InteractiveTUI::selectSensor(const SensorInfo& info) {
    currentSensor = registry.createPlugin(info.type);
    if (!currentSensor) {
        showError("Failed to create sensor plugin");
        return false;
    }
    
    if (!currentSensor->initialize(info.port)) {
        // Provide detailed error information including permission details
        std::string error_msg = "Failed to initialize sensor at " + info.port;
        
        // Add permission details if available
        if (!info.device_perms.error_message.empty()) {
            error_msg += ". " + info.device_perms.error_message;
        }
        
        // Add helpful suggestions based on permission status
        if (info.device_perms.exists && (!info.device_perms.readable || !info.device_perms.writable)) {
            error_msg += ". Try: sudo chmod 666 " + info.port + " or add user to dialout group";
        }
        
        showError(error_msg);
        currentSensor.reset();
        return false;
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

std::vector<SensorInfo> InteractiveTUI::getAvailableSensors() const {
    std::vector<SensorInfo> available;
    for (const auto& sensor : cachedSensors) {
        if (sensor.available) {
            available.push_back(sensor);
        }
    }
    return available;
}

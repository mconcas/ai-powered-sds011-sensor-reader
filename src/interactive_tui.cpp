#include "interactive_tui.h"
#include "sds011_plugin.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <thread>
#include <locale.h>

InteractiveTUI::InteractiveTUI() 
    : mainWin(nullptr), headerWin(nullptr), menuWin(nullptr), 
      dataWin(nullptr), statsWin(nullptr), statusWin(nullptr),
      currentSensor(nullptr), inSensorMode(false) {
    
    // Register available sensor plugins
    registry.registerPlugin(std::unique_ptr<SensorPlugin>(new SDS011Plugin()));
}

InteractiveTUI::~InteractiveTUI() {
    cleanup();
}

bool InteractiveTUI::initialize() {
    // Set UTF-8 locale for proper Unicode support
    setlocale(LC_ALL, "");
    
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

void InteractiveTUI::createWindows() {
    // Header window (top 3 lines)
    headerWin = newwin(3, maxX, 0, 0);
    
    if (inSensorMode) {
        // Sensor monitoring layout
        dataWin = newwin(maxY - 8, maxX, 3, 0);
        statsWin = newwin(3, maxX / 2, maxY - 5, maxX / 2);
        statusWin = newwin(2, maxX, maxY - 2, 0);
        
        scrollok(dataWin, TRUE);
        
        box(dataWin, 0, 0);
        box(statsWin, 0, 0);
        box(statusWin, 0, 0);
    } else {
        // Menu layout
        menuWin = newwin(maxY - 5, maxX, 3, 0);
        statusWin = newwin(2, maxX, maxY - 2, 0);
        
        box(menuWin, 0, 0);
        box(statusWin, 0, 0);
    }
    
    box(headerWin, 0, 0);
}

void InteractiveTUI::run() {
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

std::string InteractiveTUI::getNavigationSymbols() const {
    // Use ASCII fallback for maximum compatibility
    return "^v";
}

void InteractiveTUI::showSensorMenu() {
    // Clear and recreate windows if needed
    if (inSensorMode) {
        inSensorMode = false;
        if (dataWin) { delwin(dataWin); dataWin = nullptr; }
        if (statsWin) { delwin(statsWin); statsWin = nullptr; }
        createWindows();
    }
    
    // Draw header
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
    
    // Draw menu
    wclear(menuWin);
    box(menuWin, 0, 0);
    
    if (has_colors()) {
        wattron(menuWin, COLOR_PAIR(4) | A_BOLD);
    }
    mvwprintw(menuWin, 1, 2, "Scanning for sensors...");
    if (has_colors()) {
        wattroff(menuWin, COLOR_PAIR(4) | A_BOLD);
    }
    
    wrefresh(menuWin);
    
    // Discover sensors
    auto sensors = registry.discoverSensors();
    
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
    static int selectedIndex = 0;
    
    // Filter available sensors
    std::vector<SensorInfo> availableSensors;
    for (const auto& sensor : sensors) {
        if (sensor.available) {
            availableSensors.push_back(sensor);
        }
    }
    
    if (availableSensors.empty()) {
        if (has_colors()) {
            wattron(menuWin, COLOR_PAIR(3));
        }
        mvwprintw(menuWin, line, 2, "No sensors detected. Check connections and permissions.");
        if (has_colors()) {
            wattroff(menuWin, COLOR_PAIR(3));
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
    status << "Found " << availableSensors.size() << " available sensor(s) | "
           << "Controls: ^v Navigate, Enter Select, R Refresh, Q Quit";
    
    mvwprintw(statusWin, 1, 2, "%s", status.str().c_str());
    wrefresh(statusWin);
}

void InteractiveTUI::showSensorData() {
    if (!currentSensor) return;
    
    // Draw header
    wclear(headerWin);
    box(headerWin, 0, 0);
    
    if (has_colors()) {
        wattron(headerWin, COLOR_PAIR(4) | A_BOLD);
    }
    mvwprintw(headerWin, 1, 2, "%s - %s", 
             currentSensor->getTypeName().c_str(), 
             currentSensor->getDescription().c_str());
    mvwprintw(headerWin, 2, 2, "Port: %s | Press 'b' to go back, 'c' to clear data, 'q' to quit", 
             currentSensor->getCurrentPort().c_str());
    if (has_colors()) {
        wattroff(headerWin, COLOR_PAIR(4) | A_BOLD);
    }
    wrefresh(headerWin);
    
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
    static int selectedIndex = 0;
    
    int ch = getch();
    switch (ch) {
        case 'q':
        case 'Q':
            return 1; // Quit
            
        case KEY_UP:
            selectedIndex = std::max(0, selectedIndex - 1);
            break;
            
        case KEY_DOWN: {
            auto sensors = registry.discoverSensors();
            std::vector<SensorInfo> available;
            for (const auto& s : sensors) {
                if (s.available) available.push_back(s);
            }
            selectedIndex = std::min(selectedIndex + 1, (int)available.size() - 1);
            break;
        }
        
        case '\n':
        case '\r':
        case KEY_ENTER: {
            auto sensors = registry.discoverSensors();
            std::vector<SensorInfo> available;
            for (const auto& s : sensors) {
                if (s.available) available.push_back(s);
            }
            
            if (selectedIndex >= 0 && selectedIndex < (int)available.size()) {
                if (selectSensor(available[selectedIndex])) {
                    inSensorMode = true;
                    if (menuWin) { delwin(menuWin); menuWin = nullptr; }
                    createWindows();
                }
            }
            break;
        }
        
        case 'r':
        case 'R':
            // Refresh sensor list - just redraw
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
            
        case 'b':
        case 'B':
            // Go back to menu
            inSensorMode = false;
            currentSensor.reset();
            clearData();
            if (dataWin) { delwin(dataWin); dataWin = nullptr; }
            if (statsWin) { delwin(statsWin); statsWin = nullptr; }
            createWindows();
            break;
            
        case 'c':
        case 'C':
            clearData();
            break;
            
        case KEY_RESIZE:
            getmaxyx(stdscr, maxY, maxX);
            createWindows();
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
        showError("Failed to initialize sensor at " + info.port);
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
    if (headerWin) delwin(headerWin);
    if (menuWin) delwin(menuWin);
    if (dataWin) delwin(dataWin);
    if (statsWin) delwin(statsWin);
    if (statusWin) delwin(statusWin);
    
    if (currentSensor) {
        currentSensor->cleanup();
        currentSensor.reset();
    }
    
    if (mainWin) {
        endwin();
    }
}

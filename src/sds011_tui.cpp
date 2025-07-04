#include "sds011_tui.h"
#include "app_utils.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <algorithm>

SDS011TUI::SDS011TUI() : mainWin(nullptr), headerWin(nullptr), dataWin(nullptr), 
                          statsWin(nullptr), statusWin(nullptr) {}

SDS011TUI::~SDS011TUI() {
    cleanup();
}

bool SDS011TUI::initialize() {
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

void SDS011TUI::createWindows() {
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

void SDS011TUI::cleanup() {
    if (headerWin) delwin(headerWin);
    if (dataWin) delwin(dataWin);
    if (statsWin) delwin(statsWin);
    if (statusWin) delwin(statusWin);
    
    if (mainWin) {
        endwin();
    }
}

void SDS011TUI::drawHeader(const std::string& port) {
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

void SDS011TUI::addReading(float pm25, float pm10) {
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

void SDS011TUI::updateDataWindow() {
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
        
        mvwprintw(dataWin, line, 2, "%02d:%02d:%02d   %-8s      %-8s      %-8s",
                 tm.tm_hour, tm.tm_min, tm.tm_sec,
                 AppUtils::formatFloat(it->pm25).c_str(), 
                 AppUtils::formatFloat(it->pm10).c_str(), 
                 quality.c_str());
        
        if (has_colors()) {
            wattroff(dataWin, COLOR_PAIR(colorPair));
        }
    }
    
    wrefresh(dataWin);
}

void SDS011TUI::updateStatsWindow() {
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
    
    mvwprintw(statsWin, 1, 2, "PM2.5: Avg %s Min %s Max %s", 
              AppUtils::formatFloat(avgPM25).c_str(),
              AppUtils::formatFloat(minPM25).c_str(),
              AppUtils::formatFloat(maxPM25).c_str());
    mvwprintw(statsWin, 2, 2, "PM10:  Avg %s Min %s Max %s", 
              AppUtils::formatFloat(avgPM10).c_str(),
              AppUtils::formatFloat(minPM10).c_str(),
              AppUtils::formatFloat(maxPM10).c_str());
    
    wrefresh(statsWin);
}

void SDS011TUI::updateStatusWindow() {
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

void SDS011TUI::showError(const std::string& message) {
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

void SDS011TUI::clearData() {
    readings.clear();
    updateDataWindow();
    updateStatsWindow();
    updateStatusWindow();
}

int SDS011TUI::handleInput() {
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

#include "../include/sds011_tui.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <signal.h>

/**
 * @brief Mock TUI test program
 * 
 * This program demonstrates the TUI functionality without requiring
 * an actual SDS011 sensor by generating fake sensor data.
 */

// Global flag for clean shutdown
volatile bool g_running = true;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_running = false;
    }
}

class MockSensorTUI {
private:
    WINDOW* mainWin;
    WINDOW* headerWin;
    WINDOW* dataWin;
    WINDOW* statusWin;
    
    int maxY, maxX;
    int readingCount;
    
    std::mt19937 rng;
    std::uniform_real_distribution<float> pm25_dist;
    std::uniform_real_distribution<float> pm10_dist;
    
public:
    MockSensorTUI() : mainWin(nullptr), headerWin(nullptr), dataWin(nullptr), 
                      statusWin(nullptr), readingCount(0),
                      rng(std::chrono::steady_clock::now().time_since_epoch().count()),
                      pm25_dist(5.0, 35.0), pm10_dist(10.0, 50.0) {}
    
    ~MockSensorTUI() {
        cleanup();
    }
    
    bool initialize() {
        mainWin = initscr();
        if (mainWin == nullptr) {
            return false;
        }
        
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        timeout(100);
        
        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_GREEN, COLOR_BLACK);
            init_pair(2, COLOR_YELLOW, COLOR_BLACK);
            init_pair(3, COLOR_RED, COLOR_BLACK);
            init_pair(4, COLOR_CYAN, COLOR_BLACK);
            init_pair(5, COLOR_WHITE, COLOR_BLUE);
        }
        
        getmaxyx(stdscr, maxY, maxX);
        createWindows();
        
        return true;
    }
    
    void createWindows() {
        headerWin = newwin(3, maxX, 0, 0);
        dataWin = newwin(maxY - 6, maxX, 3, 0);
        statusWin = newwin(3, maxX, maxY - 3, 0);
        
        scrollok(dataWin, TRUE);
        
        box(headerWin, 0, 0);
        box(dataWin, 0, 0);
        box(statusWin, 0, 0);
    }
    
    void cleanup() {
        if (headerWin) delwin(headerWin);
        if (dataWin) delwin(dataWin);
        if (statusWin) delwin(statusWin);
        
        if (mainWin) {
            endwin();
        }
    }
    
    void drawHeader() {
        wclear(headerWin);
        box(headerWin, 0, 0);
        
        if (has_colors()) {
            wattron(headerWin, COLOR_PAIR(4) | A_BOLD);
        }
        
        mvwprintw(headerWin, 1, 2, "SDS011 Sensor TUI Demo - Mock Data");
        mvwprintw(headerWin, 2, 2, "Press 'q' to quit | This is a demonstration with fake data");
        
        if (has_colors()) {
            wattroff(headerWin, COLOR_PAIR(4) | A_BOLD);
        }
        
        wrefresh(headerWin);
    }
    
    void addMockReading() {
        float pm25 = pm25_dist(rng);
        float pm10 = pm10_dist(rng);
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        // Color based on PM2.5 levels
        int colorPair = 1; // Green
        std::string quality = "Good";
        
        if (pm25 > 15.0) {
            colorPair = 2; // Yellow
            quality = "Moderate";
        }
        if (pm25 > 25.0) {
            colorPair = 3; // Red
            quality = "Poor";
        }
        
        // Scroll data window and add new line
        scroll(dataWin);
        
        if (has_colors()) {
            wattron(dataWin, COLOR_PAIR(colorPair));
        }
        
        mvwprintw(dataWin, maxY - 10, 2, "%02d:%02d:%02d   PM2.5: %5.1f   PM10: %5.1f   Quality: %-8s",
                 tm.tm_hour, tm.tm_min, tm.tm_sec, pm25, pm10, quality.c_str());
        
        if (has_colors()) {
            wattroff(dataWin, COLOR_PAIR(colorPair));
        }
        
        box(dataWin, 0, 0);
        wrefresh(dataWin);
        
        readingCount++;
        updateStatusWindow();
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
        
        mvwprintw(statusWin, 1, 2, "Status: Running | Last update: %02d:%02d:%02d | Total readings: %d",
                 tm.tm_hour, tm.tm_min, tm.tm_sec, readingCount);
        mvwprintw(statusWin, 2, 2, "This is a demo with mock data - Press 'q' to quit");
        
        if (has_colors()) {
            wattroff(statusWin, COLOR_PAIR(5));
        }
        
        wrefresh(statusWin);
    }
    
    int handleInput() {
        int ch = getch();
        switch (ch) {
            case 'q':
            case 'Q':
                return 1;
            case KEY_RESIZE:
                getmaxyx(stdscr, maxY, maxX);
                wresize(headerWin, 3, maxX);
                wresize(dataWin, maxY - 6, maxX);
                wresize(statusWin, 3, maxX);
                mvwin(statusWin, maxY - 3, 0);
                
                drawHeader();
                updateStatusWindow();
                break;
        }
        return 0;
    }
};

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    MockSensorTUI tui;
    if (!tui.initialize()) {
        std::cerr << "Failed to initialize TUI" << std::endl;
        return 1;
    }
    
    tui.drawHeader();
    
    while (g_running) {
        tui.addMockReading();
        
        if (tui.handleInput() == 1) {
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    return 0;
}

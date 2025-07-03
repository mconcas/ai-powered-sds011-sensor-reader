#include <ncurses.h>
#include <iostream>

int main() {
    std::cout << "Testing ncurses..." << std::endl;
    
    // Initialize ncurses
    WINDOW* mainWin = initscr();
    if (mainWin == nullptr) {
        std::cout << "Failed to initialize ncurses" << std::endl;
        return 1;
    }
    
    // Configure ncurses
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);
    
    // Test basic output
    mvprintw(0, 0, "Hello, ncurses!");
    mvprintw(1, 0, "Press any key to exit...");
    refresh();
    
    getch();
    endwin();
    
    std::cout << "ncurses test completed" << std::endl;
    return 0;
}

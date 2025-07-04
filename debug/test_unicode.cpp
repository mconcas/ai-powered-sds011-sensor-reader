#include <iostream>
#include <locale.h>
#include <string.h>

std::string getNavigationSymbols() {
    // Check if locale supports UTF-8
    const char* locale = setlocale(LC_CTYPE, nullptr);
    if (locale && (strstr(locale, "UTF-8") || strstr(locale, "utf8"))) {
        // Try Unicode arrows
        return "↑↓";
    }
    
    // Fallback to ASCII
    return "^v";
}

int main() {
    setlocale(LC_ALL, "");
    
    std::cout << "Current locale: " << setlocale(LC_CTYPE, nullptr) << std::endl;
    std::cout << "Navigation symbols: " << getNavigationSymbols() << std::endl;
    std::cout << "Direct Unicode test: ↑↓ ←→ µg/m³" << std::endl;
    
    return 0;
}

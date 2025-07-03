#include "plugin_manager.h"
#include <iostream>

int main() {
    std::cout << "Testing plugin loading step by step..." << std::endl;
    
    try {
        std::cout << "1. Creating PluginManager..." << std::endl;
        PluginManager pluginManager("./build/plugins");
        
        std::cout << "2. Loading plugins..." << std::endl;
        bool success = pluginManager.loadAllPlugins();
        std::cout << "   Load result: " << (success ? "success" : "failed") << std::endl;
        
        std::cout << "3. Getting plugin names..." << std::endl;
        auto pluginNames = pluginManager.getPluginNames();
        std::cout << "   Found " << pluginNames.size() << " plugins" << std::endl;
        
        for (const auto& name : pluginNames) {
            std::cout << "   - " << name << std::endl;
        }
        
        std::cout << "4. Testing device detection..." << std::endl;
        auto devices = pluginManager.detectAllDevices();
        std::cout << "   Device detection completed, found " << devices.size() << " devices" << std::endl;
        
        std::cout << "5. Test completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught" << std::endl;
        return 1;
    }
    
    return 0;
}

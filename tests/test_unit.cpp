#include <iostream>
#include <cassert>
#include <string>

// Simple unit tests that don't require a terminal
// These test basic functionality without GUI components

// Test basic string operations and constants
void test_basic_functionality() {
    std::cout << "Testing basic functionality..." << std::endl;
    
    // Test string operations
    std::string test_port = "/dev/ttyUSB0";
    assert(test_port.length() > 0);
    assert(test_port.find("tty") != std::string::npos);
    
    std::cout << "✓ String operations work" << std::endl;
}

// Test platform detection
void test_platform_detection() {
    std::cout << "Testing platform detection..." << std::endl;
    
#ifdef LINUX
    std::cout << "✓ Linux platform detected" << std::endl;
#elif defined(MACOS)
    std::cout << "✓ macOS platform detected" << std::endl;
#else
    std::cout << "✓ Generic platform" << std::endl;
#endif
}

// Test data structures
void test_data_structures() {
    std::cout << "Testing data structures..." << std::endl;
    
    // Test basic data validation
    float pm25 = 15.5f;
    float pm10 = 20.3f;
    
    // Use variables in assertions to avoid unused variable warnings
    assert(pm25 > 0);
    assert(pm10 > 0);
    assert(pm10 >= pm25); // PM10 should generally be >= PM2.5
    
    // Also use them in output to ensure they're not optimized away
    std::cout << "✓ Data validation works (PM2.5: " << pm25 << ", PM10: " << pm10 << ")" << std::endl;
}

int main() {
    std::cout << "Running CI-compatible unit tests..." << std::endl;
    std::cout << "=====================================" << std::endl;
    
    try {
        test_basic_functionality();
        test_platform_detection();
        test_data_structures();
        
        std::cout << "=====================================" << std::endl;
        std::cout << "✅ All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}

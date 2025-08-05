cmake_minimum_required(VERSION 3.10)
project(MultiSensorReader VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build configuration
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# Platform detection
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOS TRUE)
    add_definitions(-DMACOS)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LINUX TRUE)
    add_definitions(-DLINUX)
else()
    message(FATAL_ERROR "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
endif()

# Find required packages
find_package(PkgConfig REQUIRED)

# Find ncurses (cross-platform)
if(MACOS)
    # On macOS, try to find ncurses via pkg-config first, then system paths
    pkg_check_modules(NCURSES ncurses)
    if(NOT NCURSES_FOUND)
        find_library(NCURSES_LIBRARIES NAMES ncurses PATHS /usr/lib /usr/local/lib /opt/homebrew/lib)
        find_path(NCURSES_INCLUDE_DIRS NAMES ncurses.h PATHS /usr/include /usr/local/include /opt/homebrew/include)
        if(NCURSES_LIBRARIES AND NCURSES_INCLUDE_DIRS)
            set(NCURSES_FOUND TRUE)
        endif()
    endif()
elseif(LINUX)
    pkg_check_modules(NCURSES REQUIRED ncurses)
endif()

if(NOT NCURSES_FOUND)
    message(FATAL_ERROR "ncurses library not found. Please install ncurses development package.")
endif()

# Include directories
include_directories(include)
include_directories(${NCURSES_INCLUDE_DIRS})

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

# Test source files
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

# Main executable
add_executable(sensor_reader ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(sensor_reader dl ${NCURSES_LIBRARIES})

# Test executables
if(TEST_SOURCES)
    # TUI test (requires terminal)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_tui.cpp")
        add_executable(test_tui tests/test_tui.cpp ${HEADERS})
        target_link_libraries(test_tui ${NCURSES_LIBRARIES})
    endif()
    
    # Unit test (CI-compatible)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_unit.cpp")
        add_executable(test_unit tests/test_unit.cpp)
        
        # Enable testing
        enable_testing()
        add_test(NAME UnitTest COMMAND test_unit)
    endif()
endif()

# Installation
install(TARGETS sensor_reader DESTINATION bin)

# Package configuration
set(CPACK_PACKAGE_NAME "MultiSensorReader")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Multi-Sensor Reader with Plugin Architecture")
set(CPACK_PACKAGE_VENDOR "AI-Generated Project")

# Set license file if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()

# Set readme file if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
endif()

# Platform-specific package settings
if(MACOS)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_FORMAT "UDBZ")
    set(CPACK_DMG_VOLUME_NAME "MultiSensorReader")
elseif(LINUX)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libncurses5")
    set(CPACK_RPM_PACKAGE_REQUIRES "ncurses-devel")
endif()

include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "=== Build Configuration Summary ===")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "ncurses found: ${NCURSES_FOUND}")
if(NCURSES_FOUND)
    message(STATUS "ncurses libraries: ${NCURSES_LIBRARIES}")
    message(STATUS "ncurses includes: ${NCURSES_INCLUDE_DIRS}")
endif()
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "===================================")
message(STATUS "")

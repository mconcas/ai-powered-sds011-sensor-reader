cmake_minimum_required(VERSION 3.10)
project(MultiSensorReader VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
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

# Core library source files (excluding plugin-specific files)
set(LIB_SOURCES
    src/app_utils.cpp
    src/interactive_tui.cpp
    src/plugin_manager.cpp
    src/sensor_registry.cpp
    src/sds011_reader.cpp  # Keep for compatibility until fully migrated
)

set(LIB_HEADERS
    include/app_utils.h
    include/interactive_tui.h
    include/plugin_interface.h
    include/plugin_manager.h
    include/sensor_plugin.h
    include/sensor_registry.h
    include/sds011_reader.h  # Keep for compatibility
)

# Core sensor library (no plugins)
add_library(sensor_lib STATIC ${LIB_SOURCES} ${LIB_HEADERS})
target_include_directories(sensor_lib PUBLIC include)
target_link_libraries(sensor_lib PRIVATE ${NCURSES_LIBRARIES})

# Add dynamic loading support
if(MACOS)
    target_link_libraries(sensor_lib PRIVATE ${CMAKE_DL_LIBS})
elseif(LINUX)
    target_link_libraries(sensor_lib PRIVATE dl)
endif()

# Create plugins directory in build
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)

# SDS011 Plugin as shared library
add_library(sds011_plugin SHARED 
    plugins/sds011/sds011_plugin.cpp
    src/sds011_reader.cpp  # Include the SDS011Reader implementation
)
target_include_directories(sds011_plugin PRIVATE include)
target_link_libraries(sds011_plugin PRIVATE ${NCURSES_LIBRARIES})
set_target_properties(sds011_plugin PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
if(MACOS)
    set_target_properties(sds011_plugin PROPERTIES SUFFIX ".dylib")
else()
    set_target_properties(sds011_plugin PROPERTIES SUFFIX ".so")
endif()

# Main executable
add_executable(sensor_reader src/main.cpp)
target_link_libraries(sensor_reader PRIVATE sensor_lib)

# Test executables
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_tui.cpp")
    add_executable(test_tui tests/test_tui.cpp)
    target_link_libraries(test_tui PRIVATE sensor_lib)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_unit.cpp")
    add_executable(test_unit tests/test_unit.cpp)
    target_link_libraries(test_unit PRIVATE sensor_lib)
    
    # Enable testing
    enable_testing()
    add_test(NAME UnitTest COMMAND test_unit)
endif()

# Plugin system test
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_system.cpp")
    add_executable(test_plugin_system test_plugin_system.cpp)
    target_link_libraries(test_plugin_system PRIVATE sensor_lib)
endif()

# Installation
install(TARGETS sensor_reader DESTINATION bin)
install(TARGETS sds011_plugin DESTINATION lib/sensor_reader/plugins)
install(DIRECTORY DESTINATION lib/sensor_reader/plugins)

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

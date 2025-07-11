# Multi-Sensor Reader with Plugin Architecture

> **⚠️ AI-Generated Project Disclaimer**
> 
> This entire project has been created by AI (GitHub Copilot) without any human writing a single line of code or reading any user manuals. The implementation is based on publicly available protocol specifications and common programming patterns. While the code follows established standards and best practices, users should thoroughly test and validate the functionality before relying on it for critical applications.

A C++ application to read data from multiple sensor types with both console and TUI (Text User Interface) modes. Currently supports SDS011 PM2.5 sensors with an extensible plugin architecture for adding new sensor types.

## Features

- **Interactive Mode**: Modern sensor selection interface with auto-discovery
  - Automatic sensor detection across multiple ports
  - Plugin-based architecture for different sensor types
  - Real-time sensor monitoring with intuitive controls
  - Easy switching between different sensors

- **Plugin Architecture**: Extensible sensor support system
  - SDS011 PM2.5/PM10 sensor plugin included
  - Easy to add new sensor types as separate plugins
  - Standardized sensor data interface
  - Type-specific display formatting and quality indicators

- **Legacy TUI Mode**: Original single-sensor interface
  - Color-coded air quality indicators (Green/Yellow/Red)
  - Real-time statistics (average, min, max values)
  - Scrolling data history
  - Keyboard controls for interaction

- **Console Mode**: Traditional command-line output for scripting and logging

- **Cross-platform**: Works on Linux and macOS systems with ncurses support

## Requirements

- C++11 compatible compiler (g++ or clang++)
- CMake 3.10 or higher
- ncurses development library
- SDS011 PM2.5 sensor connected via USB

## Installation

### Option 1: Using the Build Script (Recommended)

The easiest way to build on both Linux and macOS:

```bash
# Install dependencies and build everything
./build.sh all

# Or step by step:
./build.sh deps    # Install system dependencies
./build.sh build   # Build the project
./build.sh test    # Run tests
./build.sh install # Install system-wide
```

### Option 2: Manual Installation

#### Linux (Ubuntu/Debian):
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libncurses5-dev pkg-config
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

#### Linux (Red Hat/CentOS/Fedora):
```bash
sudo dnf install gcc-c++ cmake ncurses-devel pkgconfig
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

#### macOS:
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake ncurses pkg-config

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
sudo make install
```

### Option 3: Legacy Makefile (Linux only)
```bash
make
```

## Usage

### Interactive Mode (default):
```bash
./sensor_reader                    # Auto-detect and select sensors
```

The interactive mode will scan for available sensors and present a menu for selection. Use arrow keys to navigate and Enter to connect.

### Legacy TUI Mode:
```bash
./sensor_reader --legacy           # Legacy mode with default port

# Linux examples:
./sensor_reader --legacy /dev/ttyUSB1  # Legacy mode with custom port
./sensor_reader --legacy /dev/ttyACM0  # Legacy mode with Arduino-style port

# macOS examples:
./sensor_reader --legacy /dev/cu.usbserial-1  # Legacy mode with custom port
./sensor_reader --legacy /dev/cu.usbmodem1    # Legacy mode with modem port
```

### Console Mode:
```bash
./sensor_reader --no-tui           # Console mode with default port

# Linux examples:
./sensor_reader --no-tui /dev/ttyACM0  # Console mode with custom port

# macOS examples:
./sensor_reader --no-tui /dev/cu.usbserial # Console mode with custom port
```

### Interactive Mode Controls:
- **^v**: Navigate sensor list
- **Enter**: Connect to selected sensor
- **r**: Refresh sensor list
- **b**: Back to sensor selection (when monitoring)
- **c**: Clear collected data
- **q**: Quit the program

### Legacy TUI Controls:
- **q**: Quit the program
- **c**: Clear all collected data
- **Ctrl+C**: Emergency exit

## Interactive Mode Interface

### Sensor Selection Screen
```
┌─────────────────────────────────────────────────────────────┐
│ Interactive Sensor Monitor - Sensor Selection              │
│ Use arrow keys to select, Enter to connect, 'q' to quit   │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│ Available Sensors:                                          │
│ Port            Type       Description                      │
│ ───────────────────────────────────────────────────────────│
│ > /dev/ttyUSB0  SDS011     SDS011 PM2.5/PM10 Sensor  Connected│
│   /dev/ttyUSB1  Unknown    Unidentified device        Available│
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│ Found 1 available sensor(s) | ^v Navigate, Enter Select   │
└─────────────────────────────────────────────────────────────┘
```

### Sensor Monitoring Screen
```
┌─────────────────────────────────────────────────────────────┐
│ SDS011 - SDS011 PM2.5/PM10 Particulate Matter Sensor      │
│ Port: /dev/ttyUSB0 | Press 'b' to go back, 'c' to clear   │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│ Time       PM2.5 (µg/m³)  PM10 (µg/m³)   Quality           │
│ ─────────────────────────────────────────────────────────── │
│ 14:32:15   12.3          18.7          Good                │
│ 14:32:17   15.8          22.1          Moderate            │
│ 14:32:19   28.2          35.4          Poor                │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────┐
│ Statistics (last 50 readings)  │
│ PM2.5: Avg 18.7 Min 8.2 Max 32.1│
│ PM10:  Avg 24.3 Min 12.1 Max 45.2│
└─────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│ Status: Active | Last update: 14:32:19 | Total readings: 123│
└─────────────────────────────────────────────────────────────┘
```

## Air Quality Color Coding

- **Green**: Good (PM2.5 ≤ 15 µg/m³)
- **Yellow**: Moderate (15 < PM2.5 ≤ 25 µg/m³)  
- **Red**: Poor (PM2.5 > 25 µg/m³)

*Based on WHO Air Quality Guidelines*

## Troubleshooting

### Permission Issues:
```bash
sudo chmod 666 /dev/ttyUSB0
# or add user to dialout group:
sudo usermod -a -G dialout $USER
# (logout and login again)
```

### TUI Issues:
- If TUI fails to initialize, the program automatically falls back to console mode
- Ensure your terminal supports colors and has sufficient size
- Minimum recommended terminal size: 80x24

### Sensor Issues:
- Check USB connection
- Verify correct serial port (`ls /dev/ttyUSB*` or `ls /dev/ttyACM*`)
- Ensure sensor is powered on (fan should be running)

## Testing

A demo program is included to test the TUI without requiring the actual sensor:

```bash
g++ -std=c++11 -Wall -Wextra -O2 -o test_tui test_tui.cxx -lncurses
./test_tui
```

This generates mock sensor data to demonstrate the TUI functionality.

## Technical Details

- **Protocol**: SDS011 uses 9600 baud, 8N1 serial communication
- **Data Format**: 10-byte packets with checksum validation
- **Update Rate**: Sensor provides data approximately every second
- **Precision**: Values are provided in 0.1 µg/m³ resolution

## File Structure

### Source Code Organization
- `src/` - Source code files
  - `main.cpp` - Main application entry point
  - `interactive_tui.cpp` - Interactive sensor selection and monitoring
  - `sds011_reader.cpp` - Legacy SDS011 sensor communication (kept for compatibility)
  - `sds011_tui.cpp` - Legacy TUI interface (kept for compatibility)
  - `sds011_plugin.cpp` - SDS011 sensor plugin implementation
  - `sensor_registry.cpp` - Plugin registry and sensor discovery
  - `app_utils.cpp` - Application utilities and helpers
- `include/` - Header files
  - `interactive_tui.h` - Interactive TUI interface
  - `sensor_plugin.h` - Base sensor plugin interface
  - `sensor_registry.h` - Plugin registry and discovery
  - `sds011_plugin.h` - SDS011 sensor plugin
  - `sds011_reader.h` - Legacy SDS011 sensor reader class interface
  - `sds011_tui.h` - Legacy TUI interface class and data structures
  - `app_utils.h` - Utility functions and global definitions
- `tests/` - Test programs
  - `test_tui.cpp` - TUI demonstration with mock data
- `build/` - Build artifacts (auto-generated)
  - `obj/` - Object files for main application
  - `test_obj/` - Object files for test programs
- `Makefile` - Modern build configuration with modular support
- `README.md` - This documentation
- `.gitignore` - Git ignore rules

### Plugin Architecture
The application uses a plugin-based architecture that makes it easy to add new sensor types:

1. **SensorPlugin Interface**: Base class defining the contract for all sensors
2. **SensorData Interface**: Base class for sensor-specific data structures
3. **SensorRegistry**: Manages plugin registration and sensor discovery
4. **Plugin Implementation**: Sensor-specific implementations (e.g., SDS011Plugin)

To add a new sensor type:
1. Create a new plugin class inheriting from `SensorPlugin`
2. Implement sensor-specific data class inheriting from `SensorData`
3. Register the plugin in the main application
4. The interactive TUI will automatically discover and present the new sensor

### Build System
The project uses a modular Makefile that supports:
- Separate compilation of modules
- Automatic dependency tracking
- Test program building
- Clean build artifacts management
- Installation/uninstallation

Available make targets:
- `make` or `make all` - Build the main application
- `make test_tui` - Build the TUI test program
- `make test` - Build and run the TUI test program
- `make clean` - Remove all build artifacts
- `make install` - Install to /usr/local/bin
- `make help` - Show available targets

## Debug Tools

The `debug/` directory contains debugging tools for troubleshooting and development:

- **debug_discovery** - Tests sensor discovery mechanism
- **test_ncurses** - Basic ncurses functionality test
- **test_tui** - TUI component testing

To build and use debug tools:
```bash
cd debug
make all                    # Build all debug tools
make debug_discovery        # Build specific tool
./debug_discovery          # Run sensor discovery test
```

See `debug/README.md` for detailed information about each debug tool.

## License

This project is provided as-is for educational and personal use.

## Platform-Specific Serial Port Information

### Linux
The application automatically detects common Linux serial ports:
- `/dev/ttyUSB0`, `/dev/ttyUSB1`, etc. - USB-to-serial adapters
- `/dev/ttyACM0`, `/dev/ttyACM1`, etc. - Arduino-compatible devices
- `/dev/ttyS0`, `/dev/ttyS1` - Built-in serial ports

To list available ports:
```bash
ls /dev/tty{USB,ACM}*
```

### macOS
The application automatically detects common macOS serial ports:
- `/dev/cu.usbserial*` - USB-to-serial adapters
- `/dev/cu.usbmodem*` - USB modem devices
- `/dev/cu.SLAB_USBtoUART*` - Silicon Labs USB-to-UART bridges

To list available ports:
```bash
ls /dev/cu.*
```

**Note**: On macOS, use `cu.*` ports instead of `tty.*` ports for better compatibility.

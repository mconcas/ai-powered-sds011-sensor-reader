# Cross-Platform Development Guide

This document provides information about developing, building, and using the Multi-Sensor Reader on different platforms.

## Supported Platforms

### ✅ Linux
- **Distributions**: Ubuntu, Debian, CentOS, RHEL, Fedora, Arch Linux
- **Architectures**: x86_64, ARM64
- **Build Systems**: CMake, Make (legacy)
- **Package Managers**: apt, yum, dnf, pacman

### ✅ macOS
- **Versions**: macOS 10.15+ (Catalina and later)
- **Architectures**: x86_64, ARM64 (Apple Silicon)
- **Build Systems**: CMake
- **Package Managers**: Homebrew

### ❌ Windows
- **Status**: Not currently supported
- **Future**: May be added with WSL2 or native Windows support

## Platform Differences

### Serial Port Naming

#### Linux
```bash
# Common USB-to-serial adapters
/dev/ttyUSB0, /dev/ttyUSB1, ...

# Arduino-compatible devices
/dev/ttyACM0, /dev/ttyACM1, ...

# Built-in serial ports
/dev/ttyS0, /dev/ttyS1, ...
```

#### macOS
```bash
# USB-to-serial adapters
/dev/cu.usbserial, /dev/cu.usbserial-1, ...

# USB modem devices
/dev/cu.usbmodem, /dev/cu.usbmodem1, ...

# Silicon Labs USB-to-UART bridges
/dev/cu.SLAB_USBtoUART, /dev/cu.SLAB_USBtoUART1, ...
```

### Build Dependencies

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get install build-essential cmake libncurses5-dev pkg-config
```

#### Linux (Red Hat/CentOS/Fedora)
```bash
sudo dnf install gcc-c++ cmake ncurses-devel pkgconfig
```

#### macOS
```bash
brew install cmake ncurses pkg-config
```

## Build Options

### Option 1: Automated Build Script (Recommended)
```bash
# Full setup (installs dependencies, builds, tests, installs)
./build.sh all

# Individual steps
./build.sh deps    # Install dependencies
./build.sh build   # Build project
./build.sh test    # Run tests
./build.sh install # Install system-wide
./build.sh clean   # Clean build artifacts
```

### Option 2: Manual CMake (Cross-platform)
```bash
# Configure
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)  # Linux
make -j$(sysctl -n hw.ncpu)  # macOS

# Install
sudo make install
```

### Option 3: Legacy Makefile (Linux only)
```bash
make clean
make -j$(nproc)
```

## Development Environment Setup

### Linux Development
```bash
# Install development tools
sudo apt-get install build-essential cmake git

# Install ncurses development headers
sudo apt-get install libncurses5-dev pkg-config

# Clone and build
git clone <repository-url>
cd multi-sensor-reader
./build.sh build
```

### macOS Development
```bash
# Install Xcode command line tools
xcode-select --install

# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake ncurses pkg-config git

# Clone and build
git clone <repository-url>
cd multi-sensor-reader
./build.sh build
```

## Testing

### Automated Testing
```bash
# Run all tests
./build.sh test

# Or manually with CMake
cd build
ctest --verbose
```

### Manual Testing
```bash
# Test help output
./sensor_reader --help

# Test interactive mode (without sensor)
./sensor_reader

# Test legacy mode (without sensor)
./sensor_reader --legacy

# Test console mode (without sensor)
./sensor_reader --no-tui
```

## Troubleshooting

### Permission Issues

#### Linux
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Or temporarily change permissions
sudo chmod 666 /dev/ttyUSB0
```

#### macOS
```bash
# Usually no permission issues, but if needed:
sudo chmod 666 /dev/cu.usbserial*
```

### Build Issues

#### Missing ncurses (Linux)
```bash
# Ubuntu/Debian
sudo apt-get install libncurses5-dev

# CentOS/RHEL/Fedora
sudo dnf install ncurses-devel
```

#### Missing ncurses (macOS)
```bash
# Install via Homebrew
brew install ncurses

# If still having issues, try:
brew link ncurses --force
```

#### CMake Version Issues
```bash
# Check CMake version
cmake --version

# Upgrade if needed (Linux)
sudo apt-get install cmake

# Upgrade if needed (macOS)
brew upgrade cmake
```

### Runtime Issues

#### Serial Port Detection
```bash
# Linux: List available ports
ls /dev/tty{USB,ACM}*

# macOS: List available ports
ls /dev/cu.*
```

#### Library Loading Issues
```bash
# Check linked libraries (Linux)
ldd ./sensor_reader

# Check linked libraries (macOS)
otool -L ./sensor_reader
```

## Continuous Integration

The project uses GitHub Actions for cross-platform CI:

- **Linux**: Ubuntu latest with GCC
- **macOS**: macOS latest with Clang
- **Build Types**: Debug and Release
- **Tests**: Unit tests and integration tests

See `.github/workflows/ci.yml` for CI configuration.

## Contributing

When contributing cross-platform code:

1. **Test on both platforms** before submitting PRs
2. **Use platform-specific ifdefs** when necessary:
   ```cpp
   #ifdef MACOS
       // macOS-specific code
   #elif defined(LINUX)
       // Linux-specific code
   #else
       // Generic fallback
   #endif
   ```
3. **Update documentation** for platform-specific features
4. **Add CI tests** for new functionality

## Future Platform Support

### Windows Support
- **WSL2**: Should work with Linux instructions
- **Native Windows**: Would require:
  - Windows serial port handling (COM ports)
  - Windows-compatible ncurses alternative
  - MSVC/MinGW build configuration

### Other Unix Systems
- **FreeBSD**: Should work with minor modifications
- **OpenBSD/NetBSD**: May require serial port path updates
- **Solaris**: Possible with appropriate dependencies

# Debug Tools

This directory contains debugging tools for the multi-sensor reader project.

## Available Debug Tools

### debug_discovery
Tests the sensor discovery mechanism to verify that sensors are being detected correctly.

**Usage:**
```bash
make debug_discovery
./debug_discovery
```

**Purpose:** Verifies that the sensor registry and plugin system can detect available sensors.

### test_ncurses
Basic ncurses functionality test to ensure the terminal UI library is working correctly.

**Usage:**
```bash
make test_ncurses
./test_ncurses
```

**Purpose:** Tests basic ncurses initialization and display functionality.

### test_tui
Tests the interactive TUI components in isolation.

**Usage:**
```bash
make test_tui
./test_tui
```

**Purpose:** Tests the TUI initialization and sensor menu display.

## Building Debug Tools

Build all debug tools:
```bash
make all
```

Build a specific tool:
```bash
make debug_discovery
make test_ncurses
make test_tui
```

Clean debug programs:
```bash
make clean
```

## When to Use

These debug tools are useful for:
- Troubleshooting sensor detection issues
- Verifying ncurses functionality in different terminal environments
- Testing TUI components in isolation
- Developing new sensor plugins
- Debugging plugin registration and discovery

## Dependencies

The debug tools require the same dependencies as the main project:
- C++11 compatible compiler
- ncurses library (for TUI-related tools)
- Access to serial ports (for sensor detection tools)

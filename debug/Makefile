# Debug Tools Makefile
# This Makefile builds debugging tools for the SDS011 sensor reader project

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -I../include
LDFLAGS = -lncurses

# Source directories
SRC_DIR = ../src
DEBUG_DIR = .

# Debug programs
DEBUG_PROGRAMS = debug_discovery test_ncurses test_tui

# Default target
all: $(DEBUG_PROGRAMS)

# Debug discovery tool - tests sensor detection
debug_discovery: $(DEBUG_DIR)/debug_discovery.cpp $(SRC_DIR)/sensor_registry.cpp $(SRC_DIR)/sds011_plugin.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

# Test ncurses functionality
test_ncurses: $(DEBUG_DIR)/test_ncurses.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Test TUI functionality 
test_tui: $(DEBUG_DIR)/test_tui.cpp $(SRC_DIR)/interactive_tui.cpp $(SRC_DIR)/sensor_registry.cpp $(SRC_DIR)/sds011_plugin.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Clean debug programs
clean:
	rm -f $(DEBUG_PROGRAMS)

# Help
help:
	@echo "Debug Tools Makefile"
	@echo "===================="
	@echo "Available targets:"
	@echo "  all              - Build all debug tools"
	@echo "  debug_discovery  - Build sensor discovery test"
	@echo "  test_ncurses     - Build ncurses functionality test"
	@echo "  test_tui         - Build TUI functionality test"
	@echo "  clean            - Remove debug programs"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make all         # Build all debug tools"
	@echo "  make debug_discovery && ./debug_discovery"
	@echo "  make test_ncurses && ./test_ncurses"

.PHONY: all clean help

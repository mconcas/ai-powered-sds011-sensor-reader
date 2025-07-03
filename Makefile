# Makefile for Multi-Sensor Reader
# AI-Generated modular C++ project

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    PLATFORM_FLAGS = -DMACOS
    PLATFORM = macOS
else
    PLATFORM_FLAGS = -DLINUX
    PLATFORM = Linux
endif

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -Iinclude $(PLATFORM_FLAGS)
LDFLAGS = -lncurses

# Directories
SRC_DIR = src
INCLUDE_DIR = include
TEST_DIR = tests
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
TEST_OBJ_DIR = $(BUILD_DIR)/test_obj

# Target executables
TARGET = sensor_reader
TEST_TARGET = test_tui

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Test source files
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.cpp=$(TEST_OBJ_DIR)/%.o)

# Header files
HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)

# Default target
.PHONY: all
all: $(TARGET)

# Create build directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(TEST_OBJ_DIR):
	mkdir -p $(TEST_OBJ_DIR)

# Build main executable
$(TARGET): $(OBJECTS) | $(OBJ_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# Build object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build test executable
$(TEST_TARGET): $(TEST_OBJECTS) | $(TEST_OBJ_DIR)
	$(CXX) $(TEST_OBJECTS) -o $@ $(LDFLAGS)

# Build test object files
$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp $(HEADERS) | $(TEST_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run tests
.PHONY: test
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET)

# Install to system
.PHONY: install
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

# Uninstall from system
.PHONY: uninstall
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Show help
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all       - Build the main application (default)"
	@echo "  test      - Build and run the TUI test program"
	@echo "  clean     - Remove all build artifacts"
	@echo "  install   - Install the application to /usr/local/bin"
	@echo "  uninstall - Remove the application from /usr/local/bin"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Build configuration:"
	@echo "  Compiler: $(CXX)"
	@echo "  Flags:    $(CXXFLAGS)"
	@echo "  Linker:   $(LDFLAGS)"

# Debug target to show variables
.PHONY: debug
debug:
	@echo "SOURCES: $(SOURCES)"
	@echo "OBJECTS: $(OBJECTS)"
	@echo "TEST_SOURCES: $(TEST_SOURCES)"
	@echo "TEST_OBJECTS: $(TEST_OBJECTS)"
	@echo "HEADERS: $(HEADERS)"

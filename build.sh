#!/bin/bash

# Multi-Sensor Reader Build Script
# Supports Linux and macOS cross-platform builds

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="Linux"
    PACKAGE_MANAGER="apt"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macOS"
    PACKAGE_MANAGER="brew"
else
    print_error "Unsupported operating system: $OSTYPE"
    exit 1
fi

print_status "Detected OS: $OS"

# Function to install dependencies
install_dependencies() {
    print_status "Installing dependencies for $OS..."
    
    if [[ "$OS" == "Linux" ]]; then
        # Check if we're on Ubuntu/Debian
        if command -v apt-get &> /dev/null; then
            sudo apt-get update
            sudo apt-get install -y build-essential cmake libncurses5-dev pkg-config
        # Check if we're on Red Hat/CentOS/Fedora
        elif command -v yum &> /dev/null; then
            sudo yum install -y gcc-c++ cmake ncurses-devel pkgconfig
        elif command -v dnf &> /dev/null; then
            sudo dnf install -y gcc-c++ cmake ncurses-devel pkgconfig
        else
            print_error "Unsupported Linux distribution. Please install: build-essential, cmake, ncurses-devel, pkg-config"
            exit 1
        fi
    elif [[ "$OS" == "macOS" ]]; then
        # Check if Homebrew is installed
        if ! command -v brew &> /dev/null; then
            print_error "Homebrew not found. Please install Homebrew first:"
            print_error "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
            exit 1
        fi
        
        # Install dependencies
        brew install cmake ncurses pkg-config
        
        # Install Xcode command line tools if not present
        if ! command -v gcc &> /dev/null; then
            print_status "Installing Xcode command line tools..."
            xcode-select --install
        fi
    fi
    
    print_success "Dependencies installed successfully!"
}

# Function to build the project
build_project() {
    print_status "Building Multi-Sensor Reader..."
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Configure with CMake
    print_status "Configuring build with CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=Release
    
    # Build
    print_status "Compiling project..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    cd ..
    print_success "Build completed successfully!"
}

# Function to run tests
run_tests() {
    print_status "Running tests..."
    
    if [[ -f "build/test_tui" ]]; then
        cd build
        ctest --verbose
        cd ..
        print_success "Tests passed!"
    else
        print_warning "No tests found to run"
    fi
}

# Function to install the application
install_app() {
    print_status "Installing application..."
    
    cd build
    sudo make install
    cd ..
    
    print_success "Application installed to /usr/local/bin/sensor_reader"
}

# Function to clean build artifacts
clean_build() {
    print_status "Cleaning build artifacts..."
    
    rm -rf build
    rm -f sensor_reader test_tui
    
    print_success "Build artifacts cleaned!"
}

# Function to show help
show_help() {
    echo "Multi-Sensor Reader Build Script"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  deps     Install system dependencies"
    echo "  build    Build the project (default)"
    echo "  test     Run tests"
    echo "  install  Install the application system-wide"
    echo "  clean    Clean build artifacts"
    echo "  all      Install deps, build, test, and install"
    echo "  help     Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0           # Build the project"
    echo "  $0 all       # Full setup: deps + build + test + install"
    echo "  $0 clean     # Clean build artifacts"
}

# Main script logic
case "${1:-build}" in
    "deps")
        install_dependencies
        ;;
    "build")
        build_project
        ;;
    "test")
        run_tests
        ;;
    "install")
        install_app
        ;;
    "clean")
        clean_build
        ;;
    "all")
        install_dependencies
        build_project
        run_tests
        install_app
        ;;
    "help"|"-h"|"--help")
        show_help
        ;;
    *)
        print_error "Unknown command: $1"
        show_help
        exit 1
        ;;
esac

print_success "Script completed successfully!"

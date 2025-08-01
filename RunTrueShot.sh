#!/bin/bash

echo "Building TrueShot on macOS with vcpkg..."

# Check if vcpkg toolchain exists
VCPKG_ROOT="$HOME/vcpkg"  # Adjust path if needed
TOOLCHAIN="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

if [ ! -f "$TOOLCHAIN" ]; then
    echo "vcpkg toolchain not found at $TOOLCHAIN"
    echo "Please install vcpkg or update VCPKG_ROOT path"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Configure with vcpkg
echo "Configuring with CMake and vcpkg..."
cmake .. -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# Build
echo "Building..."
make -j$(sysctl -n hw.ncpu)

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build successful!"
echo "Running game..."
./TrueShot

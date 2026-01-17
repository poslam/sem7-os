#!/bin/bash

# Build script for Temperature Monitor GUI (Lab 6)

echo "Building Temperature Monitor GUI..."

# Create build directory
mkdir -p build
cd build

# Run CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)
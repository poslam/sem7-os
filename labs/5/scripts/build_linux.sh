#!/bin/bash

# Build script for Temperature Monitor (Lab 5)

echo "Building Temperature Monitor..."

# Create build directory
mkdir -p build
cd build

# Run CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

echo ""
echo "Build complete!"
echo ""
echo "Executables:"
echo "  - bin/temp_monitor    : Main temperature monitoring server"
echo "  - bin/temp_simulator  : Temperature simulator"
echo ""
echo "Usage:"
echo "  1. Run simulator: ./bin/temp_simulator"
echo "  2. Run server: ./bin/temp_monitor --http 8080"
echo "  3. Open browser: http://localhost:8080"
echo ""

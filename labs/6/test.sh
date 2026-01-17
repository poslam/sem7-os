#!/bin/bash

# Quick test script for Lab 5 & 6
# This script checks if the system can be built and run

echo "========================================="
echo "Lab 5 & 6 - Quick Test Script"
echo "========================================="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print status
print_status() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓${NC} $2"
    else
        echo -e "${RED}✗${NC} $2"
    fi
}

# Check dependencies
echo "Checking dependencies..."
echo ""

# Check CMake
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1)
    print_status 0 "CMake: $CMAKE_VERSION"
else
    print_status 1 "CMake: NOT FOUND"
fi

# Check GCC
if command -v gcc &> /dev/null; then
    GCC_VERSION=$(gcc --version | head -n1)
    print_status 0 "GCC: $GCC_VERSION"
else
    print_status 1 "GCC: NOT FOUND"
fi

# Check pkg-config
if command -v pkg-config &> /dev/null; then
    print_status 0 "pkg-config: OK"
else
    print_status 1 "pkg-config: NOT FOUND"
fi

# Check SQLite3
if pkg-config --exists sqlite3; then
    SQLITE_VERSION=$(pkg-config --modversion sqlite3)
    print_status 0 "SQLite3: $SQLITE_VERSION"
else
    print_status 1 "SQLite3: NOT FOUND"
fi

# Check GTK3
if pkg-config --exists gtk+-3.0; then
    GTK_VERSION=$(pkg-config --modversion gtk+-3.0)
    print_status 0 "GTK+3: $GTK_VERSION"
else
    print_status 1 "GTK+3: NOT FOUND"
fi

# Check libcurl
if pkg-config --exists libcurl; then
    CURL_VERSION=$(pkg-config --modversion libcurl)
    print_status 0 "libcurl: $CURL_VERSION"
else
    print_status 1 "libcurl: NOT FOUND"
fi

# Check json-c
if pkg-config --exists json-c; then
    JSON_VERSION=$(pkg-config --modversion json-c)
    print_status 0 "json-c: $JSON_VERSION"
else
    print_status 1 "json-c: NOT FOUND"
fi
#!/bin/bash

# Quick installation script for all dependencies

echo "======================================"
echo "Installing dependencies for Labs 5 & 6"
echo "======================================"
echo ""

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Detected: Linux"
    echo ""
    
    # Check for apt (Debian/Ubuntu)
    if command -v apt-get &> /dev/null; then
        echo "Installing packages via apt-get..."
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            libsqlite3-dev \
            libgtk-3-dev \
            libcurl4-openssl-dev \
            libjson-c-dev \
            pkg-config
    
    # Check for yum (CentOS/RHEL)
    elif command -v yum &> /dev/null; then
        echo "Installing packages via yum..."
        sudo yum install -y \
            gcc \
            gcc-c++ \
            make \
            cmake \
            sqlite-devel \
            gtk3-devel \
            libcurl-devel \
            json-c-devel \
            pkgconfig
    
    # Check for dnf (Fedora)
    elif command -v dnf &> /dev/null; then
        echo "Installing packages via dnf..."
        sudo dnf install -y \
            gcc \
            gcc-c++ \
            make \
            cmake \
            sqlite-devel \
            gtk3-devel \
            libcurl-devel \
            json-c-devel \
            pkgconfig
    else
        echo "Error: No supported package manager found!"
        exit 1
    fi

elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Detected: macOS"
    echo ""
    
    # Check for Homebrew
    if ! command -v brew &> /dev/null; then
        echo "Error: Homebrew not found!"
        echo "Please install Homebrew first: https://brew.sh"
        exit 1
    fi
    
    echo "Installing packages via Homebrew..."
    brew install cmake sqlite3 gtk+3 curl json-c pkg-config

else
    echo "Error: Unsupported OS!"
    exit 1
fi

echo ""
echo "======================================"
echo "Dependencies installed successfully!"
echo "======================================"
echo ""
echo "Next steps:"
echo "1. Build Lab 5: cd labs/5 && ./scripts/build_linux.sh"
echo "2. Build Lab 6: cd labs/6 && ./scripts/build_linux.sh"
echo ""

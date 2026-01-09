#!/bin/bash
set -e

REPO_DIR="/Users/poslam/Downloads/projects/fefu/7/os/1/sem7-os-proj"
BUILD_DIR="$REPO_DIR/build"
CONFIG="Release"

echo "=== STEP 1: Updating repo from Git ==="
cd "$REPO_DIR"
git pull

echo "=== STEP 2: Cleaning build directory ==="
if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"

echo "=== STEP 3: Configure with CMake ==="
cd "$BUILD_DIR"
cmake -DCMAKE_BUILD_TYPE=$CONFIG ..

echo "=== STEP 4: Build project ==="
make

echo "=== DONE ==="

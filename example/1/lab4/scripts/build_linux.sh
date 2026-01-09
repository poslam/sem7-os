#!/usr/bin/env bash
set -euo pipefail

# Usage: ./lab4/scripts/build_linux.sh [BuildType]
# Default: Debug. Requires git, cmake, and a C++17 toolchain in PATH.

BUILD_TYPE="${1:-Debug}"
SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
LAB4_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
REPO_ROOT="$(cd -- "${LAB4_ROOT}/.." && pwd -P)"

echo "Updating repository..."
git -C "${REPO_ROOT}" pull --rebase

echo "Configuring CMake (${BUILD_TYPE})..."
cmake -S "${LAB4_ROOT}" -B "${LAB4_ROOT}/build" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "Building project..."
cmake --build "${LAB4_ROOT}/build" --config "${BUILD_TYPE}"

echo "Done. Binaries in ${LAB4_ROOT}/build/bin"

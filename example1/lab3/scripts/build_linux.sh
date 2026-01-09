#!/usr/bin/env bash
set -euo pipefail

# Usage: ./lab3/scripts/build_linux.sh [BuildType]
# Default: Debug. Requires git, cmake, and a C++17 toolchain in PATH.

BUILD_TYPE="${1:-Debug}"
SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
LAB3_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
REPO_ROOT="$(cd -- "${LAB3_ROOT}/.." && pwd -P)"

echo "Updating repository..."
git -C "${REPO_ROOT}" pull --rebase

echo "Configuring CMake (${BUILD_TYPE})..."
cmake -S "${LAB3_ROOT}" -B "${LAB3_ROOT}/build" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "Building project..."
cmake --build "${LAB3_ROOT}/build" --config "${BUILD_TYPE}"

echo "Done. Binaries in ${LAB3_ROOT}/build/bin"

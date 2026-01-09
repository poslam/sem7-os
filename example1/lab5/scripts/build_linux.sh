#!/usr/bin/env bash
set -euo pipefail

# Usage: ./lab5/scripts/build_linux.sh [BuildType]
# Default: Debug. Requires git, cmake, C++17 toolchain, sqlite3 dev package.

BUILD_TYPE="${1:-Debug}"
SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
LAB5_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
REPO_ROOT="$(cd -- "${LAB5_ROOT}/.." && pwd -P)"

echo "Updating repository..."
git -C "${REPO_ROOT}" pull --rebase

echo "Configuring CMake (${BUILD_TYPE})..."
cmake -S "${LAB5_ROOT}" -B "${LAB5_ROOT}/build" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "Building project..."
cmake --build "${LAB5_ROOT}/build" --config "${BUILD_TYPE}"

echo "Done. Binaries in ${LAB5_ROOT}/build/bin"

#!/usr/bin/env bash
set -euo pipefail

# Usage: ./scripts/build_linux.sh [BuildType]
# Default build type is Debug. Requires git, cmake, and a C++ toolchain in PATH.

BUILD_TYPE="${1:-Debug}"

SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
LAB1_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
REPO_ROOT="$(cd -- "${LAB1_ROOT}/.." && pwd -P)"

echo "Updating repository..."
git -C "${REPO_ROOT}" pull --rebase

echo "Configuring CMake (${BUILD_TYPE})..."
cmake -S "${LAB1_ROOT}" -B "${LAB1_ROOT}/build" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "Building project..."
cmake --build "${LAB1_ROOT}/build" --config "${BUILD_TYPE}"

echo "Done. Executable in ${LAB1_ROOT}/build/bin"

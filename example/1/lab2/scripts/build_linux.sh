#!/usr/bin/env bash
set -euo pipefail

# Usage: ./scripts/build_linux.sh [BuildType]
# Default: Debug. Requires git, cmake, and a C++17 toolchain in PATH.

BUILD_TYPE="${1:-Debug}"

SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
LAB2_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
REPO_ROOT="$(cd -- "${LAB2_ROOT}/.." && pwd -P)"

echo "Updating repository..."
git -C "${REPO_ROOT}" pull --rebase

echo "Configuring CMake (${BUILD_TYPE})..."
cmake -S "${LAB2_ROOT}" -B "${LAB2_ROOT}/build" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "Building project..."
cmake --build "${LAB2_ROOT}/build" --config "${BUILD_TYPE}"

echo "Done. Binaries in ${LAB2_ROOT}/build/bin"

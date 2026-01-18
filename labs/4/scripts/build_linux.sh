#!/bin/bash

# Переходим в корень проекта (родительская директория от scripts)
cd "$(dirname "$0")/.."

mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release

# Определяем количество ядер (кроссплатформенно)
if command -v nproc &> /dev/null; then
    CORES=$(nproc)
elif command -v sysctl &> /dev/null; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=4
fi

make -j${CORES}
#!/bin/bash

# Скрипт сборки для лабораторной работы 2

echo "=========================================="
echo "Building Process Library - Lab 2"
echo "=========================================="

# Создаем директорию для сборки
mkdir -p build
cd build

# Запускаем CMake
echo "Running CMake..."
cmake ..

if [ $? -ne 0 ]; then
    echo "CMake failed!"
    exit 1
fi

# Компилируем проект
echo "Building project..."
cmake --build .

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "=========================================="
echo "Build completed successfully!"
echo "=========================================="
echo ""
echo "Executables are in: build/bin/"
echo ""
echo "To run tests:"
echo "  cd build/bin"
echo "  ./test_code"
echo ""

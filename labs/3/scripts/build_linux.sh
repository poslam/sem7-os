#!/bin/bash

# Скрипт сборки для лабораторной работы 3

echo "=========================================="
echo "Building Lab 3 - Process Manager"
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
echo "Executable is in: build/bin/"
echo ""
echo "To run:"
echo "  cd build/bin"
echo "  ./lab3_main"
echo ""
echo "To run multiple instances:"
echo "  ./lab3_main &"
echo "  ./lab3_main &"
echo ""
echo "Check logs in: build/bin/lab3/logs/process.log"
echo ""

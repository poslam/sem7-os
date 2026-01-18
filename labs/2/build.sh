#!/bin/bash

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

echo ""
echo "Executables are in: build/bin/"
echo ""
echo "To run tests:"
echo "  cd build/bin"
echo "  ./test_code"
echo ""

@echo off
REM Скрипт сборки для лабораторной работы 2 (Windows)

echo ==========================================
echo Building Process Library - Lab 2
echo ==========================================

REM Создаем директорию для сборки
if not exist build mkdir build
cd build

REM Запускаем CMake
echo Running CMake...
cmake ..

if %ERRORLEVEL% NEQ 0 (
    echo CMake failed!
    exit /b 1
)

REM Компилируем проект
echo Building project...
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo ==========================================
echo Build completed successfully!
echo ==========================================
echo.
echo Executables are in: build\bin\Release\
echo.
echo To run tests:
echo   cd build\bin\Release
echo   test_code.exe
echo.

cd ..

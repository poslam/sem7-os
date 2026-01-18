@echo off
REM Build script for Temperature Monitor (Lab 5) on Windows

echo Building Temperature Monitor...

REM Create build directory
if not exist build mkdir build
cd build

REM Clean CMake cache if exists
if exist CMakeCache.txt del CMakeCache.txt
if exist CMakeFiles rmdir /s /q CMakeFiles

REM Run CMake
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release

REM Build
make -j4

echo.
echo Build complete!
echo.
echo Executables:
echo   - bin\temp_monitor.exe    : Main temperature monitoring server
echo   - bin\temp_simulator.exe  : Temperature simulator
echo.
echo Usage:
echo   1. Run server: bin\temp_monitor.exe --http 8080
echo   2. Open browser: http://localhost:8080
echo.

pause

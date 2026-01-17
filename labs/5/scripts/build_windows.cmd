@echo off
REM Build script for Temperature Monitor (Lab 5) on Windows

echo Building Temperature Monitor...

REM Create build directory
if not exist build mkdir build
cd build

REM Run CMake
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

REM Build
mingw32-make -j4

echo.
echo Build complete!
echo.
echo Executables:
echo   - bin\temp_monitor.exe    : Main temperature monitoring server
echo   - bin\temp_simulator.exe  : Temperature simulator
echo.
echo Usage:
echo   1. Run simulator: bin\temp_simulator.exe
echo   2. Run server: bin\temp_monitor.exe --http 8080
echo   3. Open browser: http://localhost:8080
echo.

pause

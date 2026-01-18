@echo off
REM Build script for Temperature Monitor GUI (Lab 6) on Windows

echo Building Temperature Monitor GUI...

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
echo Executable: bin\temp_monitor_gui.exe
echo.
echo Usage:
echo   bin\temp_monitor_gui.exe [server_url]
echo.
echo Example:
echo   bin\temp_monitor_gui.exe http://localhost:8080
echo.
echo Note: Make sure the temperature monitor server (Lab 5) is running!
echo.

pause

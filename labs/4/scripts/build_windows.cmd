@echo off

if not exist build mkdir build
cd build

REM Run CMake
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

REM Build
mingw32-make -j4

pause

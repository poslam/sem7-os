@echo off

if not exist build mkdir build
cd build

REM Clean CMake cache if exists
if exist CMakeCache.txt del CMakeCache.txt
if exist CMakeFiles rmdir /s /q CMakeFiles

REM Run CMake
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release

REM Build
make -j4

pause

@echo off
setlocal ENABLEEXTENSIONS

REM Usage: build_windows.cmd [BuildType]
REM Default: Debug. Requires git, cmake, and MinGW-w64 g++ in PATH.

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Debug

set SCRIPT_DIR=%~dp0
for %%I in ("%SCRIPT_DIR%..") do set LAB3_ROOT=%%~fI
for %%I in ("%LAB3_ROOT%\..") do set REPO_ROOT=%%~fI

echo Updating repository...
git -C "%REPO_ROOT%" pull --rebase
if errorlevel 1 goto :error

echo Configuring CMake (%BUILD_TYPE%)...
cmake -S "%LAB3_ROOT%" -B "%LAB3_ROOT%\build" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 goto :error

echo Building project...
cmake --build "%LAB3_ROOT%\build" --config %BUILD_TYPE%
if errorlevel 1 goto :error

echo Completed. Binaries in %LAB3_ROOT%\build\bin
exit /b 0

:error
echo Build failed. See output above.
exit /b 1

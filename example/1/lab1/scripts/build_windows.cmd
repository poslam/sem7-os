@echo off
setlocal ENABLEEXTENSIONS

REM Usage: build_windows.cmd [BuildType]
REM Default build type is Debug. Requires git, cmake, and MinGW toolchain in PATH.

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Debug

set SCRIPT_DIR=%~dp0
for %%I in ("%SCRIPT_DIR%..") do set LAB1_ROOT=%%~fI
for %%I in ("%LAB1_ROOT%\..") do set REPO_ROOT=%%~fI

echo Updating repository...
git -C "%REPO_ROOT%" pull --rebase
if errorlevel 1 goto :error

echo Configuring CMake (%BUILD_TYPE%)...
cmake -S "%LAB1_ROOT%" -B "%LAB1_ROOT%\build" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 goto :error

echo Building project...
cmake --build "%LAB1_ROOT%\build" --config %BUILD_TYPE%
if errorlevel 1 goto :error

echo Completed. Executable in %LAB1_ROOT%\build\bin
exit /b 0

:error
echo Build failed. See messages above.
exit /b 1

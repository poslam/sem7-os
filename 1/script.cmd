@echo off
setlocal enabledelayedexpansion

REM === Настройки ===
set REPO_DIR=C:\path\to\your\project
set BUILD_DIR=%REPO_DIR%\build
set GENERATOR="Visual Studio 17 2022"
set CONFIG=Release

echo === STEP 1: Updating repo from Git ===
cd %REPO_DIR%
git pull
if %ERRORLEVEL% neq 0 (
    echo Git pull failed!
    exit /b 1
)

echo === STEP 2: Clean build directory ===
if exist "%BUILD_DIR%" (
    echo Removing build directory...
    rmdir /S /Q "%BUILD_DIR%"
)
mkdir "%BUILD_DIR%"

echo === STEP 3: Configure with CMake ===
cd "%BUILD_DIR%"

cmake -G %GENERATOR% -DCMAKE_BUILD_TYPE=%CONFIG% ..
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

echo === STEP 4: Build project ===
cmake --build . --config %CONFIG%
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b 1
)

echo === DONE ===
pause

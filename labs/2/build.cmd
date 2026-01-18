@echo off

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

echo Executables are in: build\bin\Release\
echo.
echo To run tests:
echo   cd build\bin\Release
echo   test_code.exe
echo.

cd ..

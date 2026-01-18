@echo off
setlocal enabledelayedexpansion

echo === Quick Lab4 Test (10 seconds) ===
echo.

REM Очищаем старые логи
if exist measurements.log del measurements.log
if exist hourly.log del hourly.log
if exist daily.log del daily.log

REM Запускаем com0com для эмуляции serial портов
REM Предполагается, что com0com установлен и настроен на пару COM10-COM11
REM Или используйте любые доступные COM порты

REM Примечание: для Windows нужно либо:
REM 1. Установить com0com (https://sourceforge.net/projects/com0com/)
REM 2. Использовать реальные COM порты
REM 3. Использовать USB-Serial адаптеры

set PORT=COM10

echo Using port: %PORT%
echo.

REM Запускаем lab4
start /B build\bin\lab4.exe %PORT%

REM Ждем немного для инициализации
timeout /t 2 /nobreak >nul

REM Получаем PID процесса lab4
for /f "tokens=2" %%a in ('tasklist /fi "imagename eq lab4.exe" /fo list ^| find "PID:"') do set LAB4_PID=%%a

if defined LAB4_PID (
    echo ✅ Lab4 started (PID: !LAB4_PID!)
) else (
    echo ❌ Failed to start lab4
    goto :cleanup
)

echo ⏱️  Collecting data for 10 seconds...
echo.

timeout /t 10 /nobreak >nul

echo 🛑 Stopping lab4...
taskkill /PID %LAB4_PID% /T >nul 2>&1

echo ⏳ Waiting for graceful shutdown...
timeout /t 2 /nobreak >nul

REM Проверяем, завершился ли процесс
tasklist /FI "PID eq %LAB4_PID%" 2>nul | find "%LAB4_PID%" >nul
if not errorlevel 1 (
    echo ⚠️  Process still running, killing forcefully
    taskkill /F /PID %LAB4_PID% >nul 2>&1
) else (
    echo ✅ Lab4 terminated gracefully
)

:results
echo.
echo ===== Results =====

if exist measurements.log (
    for /f %%a in ('find /c /v "" ^< measurements.log') do set LINES=%%a
    echo ✅ measurements.log: !LINES! lines
    echo    First 5 lines:
    powershell -Command "Get-Content measurements.log -First 5 | ForEach-Object { '   ' + $_ }"
) else (
    echo ❌ measurements.log not found
)

echo.

if exist hourly.log (
    echo ✅ hourly.log:
    for /f "delims=" %%a in (hourly.log) do echo    %%a
) else (
    echo ❌ hourly.log not created
)

echo.

if exist daily.log (
    echo ✅ daily.log:
    for /f "delims=" %%a in (daily.log) do echo    %%a
) else (
    echo ❌ daily.log not created
)

:cleanup
echo.
echo ✅ Test complete!

endlocal
pause

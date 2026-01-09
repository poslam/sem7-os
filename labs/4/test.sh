#!/bin/bash

# Создаем виртуальную пару serial портов
socat -d -d pty,raw,echo=0,link=/tmp/ttyV0 pty,raw,echo=0,link=/tmp/ttyV1 &
SOCAT_PID=$!

echo "Waiting for virtual ports to be created..."
sleep 2

# Проверяем, что порты созданы
if [ ! -e /tmp/ttyV0 ] || [ ! -e /tmp/ttyV1 ]; then
    echo "Error: Failed to create virtual serial ports"
    kill $SOCAT_PID 2>/dev/null
    exit 1
fi

echo "Virtual serial ports created: /tmp/ttyV0 and /tmp/ttyV1"

# Запускаем симулятор на одном порту
echo "Starting temperature simulator..."
./simulator /tmp/ttyV0 &
SIMULATOR_PID=$!

sleep 1

# Запускаем программу на другом порту
echo "Starting lab4..."
./lab4 /tmp/ttyV1 &
LAB4_PID=$!

echo ""
echo "Running test for 30 seconds..."
echo "Simulator PID: $SIMULATOR_PID"
echo "Lab4 PID: $LAB4_PID"
echo "Socat PID: $SOCAT_PID"
echo ""

# Даем программам поработать
sleep 30

# Останавливаем все процессы
echo ""
echo "Stopping processes..."
kill $LAB4_PID 2>/dev/null
kill $SIMULATOR_PID 2>/dev/null
kill $SOCAT_PID 2>/dev/null

sleep 1

# Показываем результаты
echo ""
echo "===== Test Results ====="
echo ""

if [ -f measurements.log ]; then
    echo "measurements.log (first 10 lines):"
    head -10 measurements.log
    echo "Total lines: $(wc -l < measurements.log)"
else
    echo "ERROR: measurements.log not found!"
fi

echo ""

if [ -f hourly.log ]; then
    echo "hourly.log:"
    cat hourly.log
else
    echo "INFO: hourly.log not created (test too short)"
fi

echo ""

if [ -f daily.log ]; then
    echo "daily.log:"
    cat daily.log
else
    echo "INFO: daily.log not created (test too short)"
fi

echo ""
echo "Test complete!"

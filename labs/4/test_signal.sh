#!/bin/bash

echo "Testing signal handling..."

# Очищаем логи
rm -f measurements.log hourly.log daily.log 2>/dev/null

# Запускаем программу с несуществующим портом (будет ошибка, но это нормально для теста)
echo "Starting lab4 for 5 seconds..."
./lab4 /dev/null &
PID=$!

echo "Lab4 started with PID: $PID"
sleep 5

echo "Sending SIGINT..."
kill -INT $PID 2>/dev/null

echo "Waiting for graceful shutdown..."
sleep 2

# Проверяем, что процесс завершился
if ps -p $PID > /dev/null 2>&1; then
    echo "❌ Process still running, killing forcefully"
    kill -9 $PID 2>/dev/null
else
    echo "✅ Process terminated gracefully"
fi

echo ""
echo "Test complete!"

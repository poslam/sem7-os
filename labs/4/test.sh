#!/bin/bash

echo "=== Quick Lab4 Test (10 seconds) ==="
echo ""

# Очищаем старые логи
rm -f measurements.log hourly.log daily.log

# Создаем виртуальную пару serial портов
socat -d -d pty,raw,echo=0,link=/tmp/ttyV0 pty,raw,echo=0,link=/tmp/ttyV1 2>/dev/null &
SOCAT_PID=$!

sleep 2

if [ ! -e /tmp/ttyV0 ] || [ ! -e /tmp/ttyV1 ]; then
    echo "Failed to create virtual serial ports"
    kill $SOCAT_PID 2>/dev/null
    exit 1
fi

echo "Virtual serial ports created"

# Запускаем симулятор
./build/bin/simulator /tmp/ttyV0 2>/dev/null &
SIMULATOR_PID=$!

sleep 1

# Запускаем lab4
./build/bin/lab4 /tmp/ttyV1 &
LAB4_PID=$!

echo "✅ Lab4 started (PID: $LAB4_PID)"
echo "⏱️  Collecting data for 10 seconds..."
echo ""

sleep 10

echo "🛑 Sending SIGINT to lab4..."
kill -INT $LAB4_PID 2>/dev/null

echo "⏳ Waiting for graceful shutdown..."
sleep 2

if ps -p $LAB4_PID > /dev/null 2>&1; then
    echo "⚠️  Process still running, killing forcefully"
    kill -9 $LAB4_PID 2>/dev/null
else
    echo "✅ Lab4 terminated gracefully"
fi

kill -TERM $SIMULATOR_PID 2>/dev/null
kill -TERM $SOCAT_PID 2>/dev/null
sleep 1

echo ""
echo "===== Results ====="

if [ -f measurements.log ]; then
    LINES=$(wc -l < measurements.log)
    echo "✅ measurements.log: $LINES lines"
    echo "   First 5 lines:"
    head -5 measurements.log | sed 's/^/   /'
else
    echo "measurements.log not found"
fi

echo ""

if [ -f hourly.log ]; then
    echo "✅ hourly.log:"
    cat hourly.log | sed 's/^/   /'
else
    echo "hourly.log not created"
fi

echo ""

if [ -f daily.log ]; then
    echo "✅ daily.log:"
    cat daily.log | sed 's/^/   /'
else
    echo "daily.log not created"
fi

echo ""
echo "✅ Test complete!"

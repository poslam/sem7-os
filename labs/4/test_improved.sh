#!/bin/bash

echo "=== Lab4 Temperature Logger Test ==="
echo ""

# Обработка Ctrl+C
trap cleanup INT TERM

cleanup() {
    echo ""
    echo "🛑 Stopping processes gracefully..."
    
    # Отправляем SIGINT процессам
    if [ ! -z "$LAB4_PID" ]; then
        kill -SIGINT $LAB4_PID 2>/dev/null
    fi
    if [ ! -z "$SIMULATOR_PID" ]; then
        kill -SIGTERM $SIMULATOR_PID 2>/dev/null
    fi
    if [ ! -z "$SOCAT_PID" ]; then
        kill -SIGTERM $SOCAT_PID 2>/dev/null
    fi
    
    sleep 2
    show_results
    exit 0
}

show_results() {
    echo ""
    echo "===== 📋 Test Results ====="
    echo ""

    if [ -f measurements.log ]; then
        echo "✅ measurements.log (first 10 lines):"
        head -10 measurements.log | sed 's/^/   /'
        echo ""
        echo "   Total measurements: $(wc -l < measurements.log)"
    else
        echo "❌ ERROR: measurements.log not found!"
    fi

    echo ""

    if [ -f hourly.log ]; then
        echo "✅ hourly.log:"
        cat hourly.log | sed 's/^/   /'
    else
        echo "ℹ️  hourly.log not created (test duration < 1 hour)"
    fi

    echo ""

    if [ -f daily.log ]; then
        echo "✅ daily.log:"
        cat daily.log | sed 's/^/   /'
    else
        echo "ℹ️  daily.log not created (test duration < 1 day)"
    fi

    echo ""
    echo "✅ Test complete!"
}

# Очищаем старые логи
rm -f measurements.log hourly.log daily.log

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

echo "✅ Virtual serial ports created: /tmp/ttyV0 and /tmp/ttyV1"
echo ""

# Запускаем симулятор на одном порту
echo "Starting temperature simulator..."
./simulator /tmp/ttyV0 &
SIMULATOR_PID=$!

sleep 1

# Запускаем программу на другом порту
echo "Starting lab4 logger..."
./lab4 /tmp/ttyV1 &
LAB4_PID=$!

echo ""
echo "📊 Running test for 20 seconds..."
echo "   Simulator PID: $SIMULATOR_PID"
echo "   Lab4 PID: $LAB4_PID"
echo "   Socat PID: $SOCAT_PID"
echo ""
echo "⌨️  Press Ctrl+C to stop manually, or wait 20 seconds..."
echo ""

# Даем программам поработать
sleep 20

cleanup

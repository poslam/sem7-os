#!/bin/bash

# Скрипт для генерации тестовых данных в БД

cd "$(dirname "$0")/.."

DB_PATH="${1:-build/bin/temperature.db}"
DAYS="${2:-7}"
BASE_TEMP="${3:-22.0}"

echo "=== Temperature Monitor - Test Data Generator ==="
echo ""
echo "Database: $DB_PATH"
echo "Period: $DAYS days"
echo "Base temperature: ${BASE_TEMP}°C"
echo ""

# Проверяем наличие утилиты
if [ ! -f "build/bin/generate_test_data" ]; then
    echo "Error: generate_test_data not found. Building..."
    cd build
    cmake .. && make generate_test_data
    cd ..
fi

# Удаляем старую БД если нужно
if [ -f "$DB_PATH" ]; then
    read -p "Database exists. Remove and regenerate? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -f "$DB_PATH"
        echo "Old database removed."
    else
        echo "Keeping existing database and adding data..."
    fi
fi

# Генерируем данные
echo ""
./build/bin/generate_test_data "$DB_PATH" "$DAYS" "$BASE_TEMP"

echo ""
echo "=== Done! ==="
echo ""
echo "To view data:"
echo "  sqlite3 $DB_PATH \"SELECT COUNT(*) as readings FROM readings;\""
echo "  sqlite3 $DB_PATH \"SELECT COUNT(*) as hourly FROM hourly_avg;\""
echo "  sqlite3 $DB_PATH \"SELECT COUNT(*) as daily FROM daily_avg;\""
echo ""
echo "To start monitoring server:"
echo "  cd build/bin"
echo "  ./temp_monitor"
echo "  Open http://localhost:8080 in browser"
echo ""

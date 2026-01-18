#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sqlite3.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Генерация реалистичной температуры
double generate_temperature(time_t timestamp, double base_temp) {
    // Дневной цикл (24 часа)
    double daily_cycle = 5.0 * sin((timestamp % 86400) * 2.0 * M_PI / 86400.0);
    
    // Недельный тренд
    double weekly_trend = 2.0 * sin(timestamp * 2.0 * M_PI / (7 * 86400.0));
    
    // Случайный шум
    double noise = ((rand() % 100 - 50) / 100.0);
    
    return base_temp + daily_cycle + weekly_trend + noise;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <database_path> [days] [base_temp]\n", argv[0]);
        printf("  days: number of days to generate (default: 7)\n");
        printf("  base_temp: base temperature in °C (default: 20.0)\n");
        return 1;
    }

    const char *db_path = argv[1];
    int days = argc > 2 ? atoi(argv[2]) : 7;
    double base_temp = argc > 3 ? atof(argv[3]) : 20.0;

    printf("Generating test data for %d days with base temperature %.1f°C\n", days, base_temp);

    // Открываем БД
    sqlite3 *db;
    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Создаем таблицы если их нет
    const char *sql_create = 
        "CREATE TABLE IF NOT EXISTS readings ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "temperature REAL NOT NULL,"
        "timestamp INTEGER NOT NULL);"
        
        "CREATE TABLE IF NOT EXISTS hourly_avg ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "avg_temperature REAL NOT NULL,"
        "hour_start INTEGER NOT NULL);"
        
        "CREATE TABLE IF NOT EXISTS daily_avg ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "avg_temperature REAL NOT NULL,"
        "day_start INTEGER NOT NULL);";

    char *err_msg = NULL;
    rc = sqlite3_exec(db, sql_create, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    // Начинаем транзакцию для быстрой вставки
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    // Подготавливаем запросы
    sqlite3_stmt *stmt_reading, *stmt_hourly, *stmt_daily;
    
    sqlite3_prepare_v2(db, "INSERT INTO readings (temperature, timestamp) VALUES (?, ?);", 
                       -1, &stmt_reading, NULL);
    sqlite3_prepare_v2(db, "INSERT INTO hourly_avg (avg_temperature, hour_start) VALUES (?, ?);", 
                       -1, &stmt_hourly, NULL);
    sqlite3_prepare_v2(db, "INSERT INTO daily_avg (avg_temperature, day_start) VALUES (?, ?);", 
                       -1, &stmt_daily, NULL);

    srand(time(NULL));

    // Текущее время минус N дней
    time_t now = time(NULL);
    time_t start_time = now - (days * 86400);

    int readings_count = 0;
    int hourly_count = 0;
    int daily_count = 0;

    printf("Generating data from %s", ctime(&start_time));

    // Генерируем данные с интервалом 1 минута
    for (time_t t = start_time; t <= now; t += 60) {
        double temp = generate_temperature(t, base_temp);
        
        // Вставляем измерение
        sqlite3_reset(stmt_reading);
        sqlite3_bind_double(stmt_reading, 1, temp);
        sqlite3_bind_int64(stmt_reading, 2, (sqlite3_int64)t);
        sqlite3_step(stmt_reading);
        readings_count++;
    }

    // Теперь вычисляем агрегаты из сохраненных данных
    printf("Calculating hourly and daily averages...\n");

    // Часовые средние
    const char *sql_hourly_calc = 
        "INSERT INTO hourly_avg (avg_temperature, hour_start) "
        "SELECT AVG(temperature), (timestamp / 3600) * 3600 "
        "FROM readings "
        "GROUP BY timestamp / 3600 "
        "ORDER BY timestamp / 3600;";
    
    rc = sqlite3_exec(db, sql_hourly_calc, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Hourly calc error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        hourly_count = sqlite3_changes(db);
    }

    // Дневные средние
    const char *sql_daily_calc = 
        "INSERT INTO daily_avg (avg_temperature, day_start) "
        "SELECT AVG(temperature), (timestamp / 86400) * 86400 "
        "FROM readings "
        "GROUP BY timestamp / 86400 "
        "ORDER BY timestamp / 86400;";
    
    rc = sqlite3_exec(db, sql_daily_calc, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Daily calc error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        daily_count = sqlite3_changes(db);
    }

    printf("\n");

    // Завершаем транзакцию
    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

    // Очистка
    sqlite3_finalize(stmt_reading);
    sqlite3_finalize(stmt_hourly);
    sqlite3_finalize(stmt_daily);
    sqlite3_close(db);

    printf("\n✓ Successfully generated:\n");
    printf("  - %d readings (every minute)\n", readings_count);
    printf("  - %d hourly averages\n", hourly_count);
    printf("  - %d daily averages\n", daily_count);
    printf("\nDatabase: %s\n", db_path);

    return 0;
}

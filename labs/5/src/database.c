#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sqlite3 *db = NULL;

int db_init(const char *db_path) {
    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // Create tables
    const char *sql_readings = 
        "CREATE TABLE IF NOT EXISTS readings ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "temperature REAL NOT NULL,"
        "timestamp INTEGER NOT NULL);";
    
    const char *sql_hourly = 
        "CREATE TABLE IF NOT EXISTS hourly_avg ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "avg_temperature REAL NOT NULL,"
        "hour_start INTEGER NOT NULL);";
    
    const char *sql_daily = 
        "CREATE TABLE IF NOT EXISTS daily_avg ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "avg_temperature REAL NOT NULL,"
        "day_start INTEGER NOT NULL);";

    char *err_msg = NULL;
    rc = sqlite3_exec(db, sql_readings, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    rc = sqlite3_exec(db, sql_hourly, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    rc = sqlite3_exec(db, sql_daily, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    printf("Database initialized successfully\n");
    return 0;
}

void db_close(void) {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

int db_insert_reading(double temperature, time_t timestamp) {
    if (!db) return -1;

    const char *sql = "INSERT INTO readings (temperature, timestamp) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_double(stmt, 1, temperature);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)timestamp);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert reading: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    return 0;
}

int db_get_current_temp(double *temp, time_t *timestamp) {
    if (!db || !temp || !timestamp) return -1;

    const char *sql = "SELECT temperature, timestamp FROM readings ORDER BY timestamp DESC LIMIT 1;";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        *temp = sqlite3_column_double(stmt, 0);
        *timestamp = (time_t)sqlite3_column_int64(stmt, 1);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_get_avg_temp(time_t start, time_t end, double *avg_temp) {
    if (!db || !avg_temp) return -1;

    const char *sql = "SELECT AVG(temperature) FROM readings WHERE timestamp >= ? AND timestamp <= ?;";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)start);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)end);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        *avg_temp = sqlite3_column_double(stmt, 0);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_cleanup_old_data(void) {
    if (!db) return -1;

    time_t now = time(NULL);
    time_t cutoff = now - (24 * 3600); // 24 hours ago

    const char *sql = "DELETE FROM readings WHERE timestamp < ?;";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)cutoff);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to cleanup old data: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    return 0;
}

int db_insert_hourly_avg(double avg_temp, time_t hour_start) {
    if (!db) return -1;

    const char *sql = "INSERT INTO hourly_avg (avg_temperature, hour_start) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_double(stmt, 1, avg_temp);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)hour_start);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert hourly avg: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    return 0;
}

int db_insert_daily_avg(double avg_temp, time_t day_start) {
    if (!db) return -1;

    const char *sql = "INSERT INTO daily_avg (avg_temperature, day_start) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_double(stmt, 1, avg_temp);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)day_start);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert daily avg: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    return 0;
}

int db_get_hourly_data(time_t start, time_t end, double **temps, time_t **timestamps, int *count) {
    if (!db || !temps || !timestamps || !count) return -1;

    const char *sql = "SELECT avg_temperature, hour_start FROM hourly_avg "
                     "WHERE hour_start >= ? AND hour_start <= ? ORDER BY hour_start;";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)start);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)end);

    // Count rows first
    int row_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        row_count++;
    }
    sqlite3_reset(stmt);

    if (row_count == 0) {
        sqlite3_finalize(stmt);
        *count = 0;
        return 0;
    }

    // Allocate memory
    *temps = malloc(row_count * sizeof(double));
    *timestamps = malloc(row_count * sizeof(time_t));
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        (*temps)[i] = sqlite3_column_double(stmt, 0);
        (*timestamps)[i] = (time_t)sqlite3_column_int64(stmt, 1);
        i++;
    }

    *count = row_count;
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_daily_data(time_t start, time_t end, double **temps, time_t **timestamps, int *count) {
    if (!db || !temps || !timestamps || !count) return -1;

    const char *sql = "SELECT avg_temperature, day_start FROM daily_avg "
                     "WHERE day_start >= ? AND day_start <= ? ORDER BY day_start;";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)start);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)end);

    // Count rows first
    int row_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        row_count++;
    }
    sqlite3_reset(stmt);

    if (row_count == 0) {
        sqlite3_finalize(stmt);
        *count = 0;
        return 0;
    }

    // Allocate memory
    *temps = malloc(row_count * sizeof(double));
    *timestamps = malloc(row_count * sizeof(time_t));
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        (*temps)[i] = sqlite3_column_double(stmt, 0);
        (*timestamps)[i] = (time_t)sqlite3_column_int64(stmt, 1);
        i++;
    }

    *count = row_count;
    sqlite3_finalize(stmt);
    return 0;
}

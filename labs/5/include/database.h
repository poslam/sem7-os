#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <time.h>

// Initialize database and create tables
int db_init(const char *db_path);

// Close database connection
void db_close(void);

// Insert temperature reading
int db_insert_reading(double temperature, time_t timestamp);

// Get current temperature
int db_get_current_temp(double *temp, time_t *timestamp);

// Get average temperature for a period
int db_get_avg_temp(time_t start, time_t end, double *avg_temp);

// Clean old data (keep only last 24 hours of raw data)
int db_cleanup_old_data(void);

// Insert hourly average
int db_insert_hourly_avg(double avg_temp, time_t hour_start);

// Insert daily average
int db_insert_daily_avg(double avg_temp, time_t day_start);

// Get hourly averages for a range
int db_get_hourly_data(time_t start, time_t end, double **temps, time_t **timestamps, int *count);

// Get daily averages for a range
int db_get_daily_data(time_t start, time_t end, double **temps, time_t **timestamps, int *count);

#endif // DATABASE_H

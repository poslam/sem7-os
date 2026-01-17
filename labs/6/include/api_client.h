#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <time.h>

// Structure for temperature data point
typedef struct
{
    double temperature;
    time_t timestamp;
} TempData;

// Structure for data array
typedef struct
{
    TempData *data;
    int count;
} TempDataArray;

// Initialize API client
int api_client_init(const char *base_url);

// Get current temperature
int api_get_current_temp(double *temp, time_t *timestamp);

// Get hourly data
TempDataArray *api_get_hourly_data(void);

// Get daily data
TempDataArray *api_get_daily_data(void);

// Free data array
void api_free_data_array(TempDataArray *array);

// Cleanup API client
void api_client_cleanup(void);

#endif // API_CLIENT_H

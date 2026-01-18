#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep((x) * 1000)
#else
#include <unistd.h>
#endif

#include "database.h"
#include "http_server.h"
#include "serial_reader.h"

static volatile int running = 1;
static time_t last_hour_calc = 0;
static time_t last_day_calc = 0;

void signal_handler(int sig) {
    printf("\nShutting down...\n");
    running = 0;
}

// Thread for reading from serial port or generating test data
void* serial_reader_thread(void* arg) {
    serial_port_t port = *(serial_port_t*)arg;
    
    // Check if we're in simulation mode (port < 0 means no real device)
    int simulation_mode = (port < 0);
    double base_temp = 20.0;
    
    while (running) {
        double temperature;
        
        if (simulation_mode) {
            // Generate realistic temperature data
            time_t now = time(NULL);
            double variation = ((rand() % 100 - 50) / 50.0); // -1.0 to +1.0
            double daily_cycle = 5.0 * sin(now * 2.0 * M_PI / 86400.0); // Daily cycle
            temperature = base_temp + variation + daily_cycle;
            
            db_insert_reading(temperature, now);
            printf("Temperature (simulated): %.2f°C (timestamp: %ld)\n", temperature, (long)now);
        } else {
            // Read from real serial port
            if (serial_read_temperature(port, &temperature) == 0) {
                time_t now = time(NULL);
                db_insert_reading(temperature, now);
                printf("Temperature: %.2f°C (timestamp: %ld)\n", temperature, (long)now);
            }
        }
        
        sleep(1);
    }
    
    return NULL;
}

// Thread for HTTP server
void* http_server_thread(void* arg) {
    http_server_run();
    return NULL;
}

// Calculate and store hourly average
void calculate_hourly_average() {
    time_t now = time(NULL);
    time_t hour_start = (now / 3600) * 3600 - 3600; // Previous hour
    time_t hour_end = hour_start + 3600;
    
    double avg_temp;
    if (db_get_avg_temp(hour_start, hour_end, &avg_temp) == 0) {
        db_insert_hourly_avg(avg_temp, hour_start);
        printf("Hourly average calculated: %.2f°C\n", avg_temp);
    }
}

// Calculate and store daily average
void calculate_daily_average() {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    tm_info->tm_hour = 0;
    tm_info->tm_min = 0;
    tm_info->tm_sec = 0;
    time_t day_start = mktime(tm_info) - 86400; // Previous day
    time_t day_end = day_start + 86400;
    
    double avg_temp;
    if (db_get_avg_temp(day_start, day_end, &avg_temp) == 0) {
        db_insert_daily_avg(avg_temp, day_start);
        printf("Daily average calculated: %.2f°C\n", avg_temp);
    }
}

int main(int argc, char *argv[]) {
    // По умолчанию БД в той же директории, где исполняемый файл
    char default_db_path[1024];
    
    // Получаем директорию исполняемого файла
    const char *exe_dir = ".";
#ifndef _WIN32
    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        // На macOS /proc/self/exe не работает, используем argv[0]
        if (argc > 0 && argv[0] != NULL) {
            strncpy(exe_path, argv[0], sizeof(exe_path) - 1);
            exe_path[sizeof(exe_path) - 1] = '\0';
            char *last_slash = strrchr(exe_path, '/');
            if (last_slash != NULL) {
                *last_slash = '\0';
                exe_dir = exe_path;
            }
        }
    } else {
        exe_path[len] = '\0';
        char *last_slash = strrchr(exe_path, '/');
        if (last_slash != NULL) {
            *last_slash = '\0';
            exe_dir = exe_path;
        }
    }
#endif
    
    snprintf(default_db_path, sizeof(default_db_path), "%s/temperature.db", exe_dir);
    const char *db_path = default_db_path;
    
    const char *port_name = "/dev/ttyUSB0"; // Adjust for your system
    int http_port = 8080;
    int serial_baud = 9600;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--db") == 0 && i + 1 < argc) {
            db_path = argv[++i];
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port_name = argv[++i];
        } else if (strcmp(argv[i], "--http") == 0 && i + 1 < argc) {
            http_port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--baud") == 0 && i + 1 < argc) {
            serial_baud = atoi(argv[++i]);
        }
    }

    printf("Temperature Monitor Server\n");
    printf("Database: %s\n", db_path);
    printf("Serial port: %s\n", port_name);
    printf("HTTP port: %d\n", http_port);
    printf("\n");

    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize database
    if (db_init(db_path) != 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }

    // Open serial port
    serial_port_t serial_port = serial_open(port_name, serial_baud);
    if (serial_port < 0) {
        fprintf(stderr, "Failed to open serial port, switching to SIMULATION MODE\n");
        printf("Generating test temperature data...\n");
    } else {
        printf("Successfully opened serial port: %s\n", port_name);
    }

    // Start HTTP server
    if (http_server_start(http_port) != 0) {
        fprintf(stderr, "Failed to start HTTP server\n");
        db_close();
        return 1;
    }

    // Create threads
    pthread_t serial_thread, http_thread;
    
    // Always create serial thread (will use simulation if port < 0)
    pthread_create(&serial_thread, NULL, serial_reader_thread, &serial_port);
    pthread_create(&http_thread, NULL, http_server_thread, NULL);

    // Main loop for periodic calculations
    last_hour_calc = time(NULL);
    last_day_calc = time(NULL);

    while (running) {
        sleep(60); // Check every minute
        
        time_t now = time(NULL);
        
        // Check if an hour has passed
        if (now - last_hour_calc >= 3600) {
            calculate_hourly_average();
            last_hour_calc = now;
        }
        
        // Check if a day has passed
        if (now - last_day_calc >= 86400) {
            calculate_daily_average();
            db_cleanup_old_data(); // Clean up old raw data
            last_day_calc = now;
        }
    }

    // Cleanup
    printf("Cleaning up...\n");
    http_server_stop();
    
    pthread_join(serial_thread, NULL);
    if (serial_port >= 0) {
        serial_close(serial_port);
    }
    pthread_join(http_thread, NULL);
    
    db_close();
    
    printf("Server stopped\n");
    return 0;
}

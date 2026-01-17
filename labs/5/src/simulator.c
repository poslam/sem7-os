#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep((x) * 1000)
#else
#include <unistd.h>
#endif

#include "serial_reader.h"

// Simulate temperature sensor
int main(int argc, char *argv[]) {
    const char *port_name = argc > 1 ? argv[1] : "/tmp/virtual_serial";
    int baud_rate = argc > 2 ? atoi(argv[2]) : 9600;

    printf("Temperature simulator starting...\n");
    printf("Simulating temperature data on virtual port\n");

    // Base temperature
    double base_temp = 20.0;
    
    while (1) {
        // Generate realistic temperature with some variation
        double variation = (rand() % 100 - 50) / 50.0; // -1.0 to +1.0
        double daily_cycle = 5.0 * sin(time(NULL) * 2.0 * M_PI / 86400.0); // Daily cycle
        double temp = base_temp + variation + daily_cycle;
        
        // Print temperature in format that can be read via serial
        printf("TEMP:%.2f\n", temp);
        fflush(stdout);
        
        sleep(1); // Send data every second
    }

    return 0;
}

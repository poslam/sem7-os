#ifndef SERIAL_READER_H
#define SERIAL_READER_H

#ifdef _WIN32
#include <windows.h>
typedef HANDLE serial_port_t;
#else
typedef int serial_port_t;
#endif

// Open serial port
serial_port_t serial_open(const char *port_name, int baud_rate);

// Close serial port
void serial_close(serial_port_t port);

// Read temperature from serial port
int serial_read_temperature(serial_port_t port, double *temperature);

#endif // SERIAL_READER_H

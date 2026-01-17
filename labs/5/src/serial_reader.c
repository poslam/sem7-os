#include "serial_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>

serial_port_t serial_open(const char *port_name, int baud_rate) {
    HANDLE hSerial = CreateFile(port_name,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error opening serial port\n");
        return INVALID_HANDLE_VALUE;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        fprintf(stderr, "Error getting serial port state\n");
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    dcbSerialParams.BaudRate = baud_rate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        fprintf(stderr, "Error setting serial port state\n");
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    
    if (!SetCommTimeouts(hSerial, &timeouts)) {
        fprintf(stderr, "Error setting timeouts\n");
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    return hSerial;
}

void serial_close(serial_port_t port) {
    if (port != INVALID_HANDLE_VALUE) {
        CloseHandle(port);
    }
}

int serial_read_temperature(serial_port_t port, double *temperature) {
    if (port == INVALID_HANDLE_VALUE || !temperature) {
        return -1;
    }

    char buffer[256];
    DWORD bytes_read;
    
    if (!ReadFile(port, buffer, sizeof(buffer) - 1, &bytes_read, NULL)) {
        return -1;
    }

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        if (sscanf(buffer, "%lf", temperature) == 1) {
            return 0;
        }
    }

    return -1;
}

#else // POSIX (Linux, macOS)
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

serial_port_t serial_open(const char *port_name, int baud_rate) {
    int fd = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        fprintf(stderr, "Error opening serial port %s\n", port_name);
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);

    // Set baud rate
    speed_t speed = B9600;
    switch(baud_rate) {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        default: speed = B9600; break;
    }

    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);

    // 8N1
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    
    // No flow control
    options.c_cflag &= ~CRTSCTS;
    
    // Enable reading
    options.c_cflag |= CREAD | CLOCAL;
    
    // Raw input
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;
    
    // Set timeouts
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 10;

    tcsetattr(fd, TCSANOW, &options);
    tcflush(fd, TCIOFLUSH);

    return fd;
}

void serial_close(serial_port_t port) {
    if (port >= 0) {
        close(port);
    }
}

int serial_read_temperature(serial_port_t port, double *temperature) {
    if (port < 0 || !temperature) {
        return -1;
    }

    char buffer[256];
    int bytes_read = read(port, buffer, sizeof(buffer) - 1);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        if (sscanf(buffer, "%lf", temperature) == 1) {
            return 0;
        }
    }

    return -1;
}

#endif

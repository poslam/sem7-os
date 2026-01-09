#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep((x) * 1000)
#else
    #include <fcntl.h>
    #include <signal.h>
    #include <sys/select.h>
    #include <termios.h>
    #include <unistd.h>
#endif

#define LOG_MEASURE "measurements.log"
#define LOG_HOURLY "hourly.log"
#define LOG_DAILY "daily.log"

#define MAX_MEASUREMENTS_LOG (24 * 60 * 60)
#define MAX_HOURLY_LOG (24 * 30)
#define MAX_DAILY_LOG 365

#ifndef _WIN32
static volatile sig_atomic_t g_running = 1;
#else
static volatile int g_running = 1;
#endif

struct Measurement {
    time_t ts;
    double value;
};

struct MeasurementBuffer {
    struct Measurement items[10];
    size_t size;
};

struct RollingStats {
    double sum;
    size_t count;
};

static time_t now_ts(void) {
    return time(NULL);
}

static struct tm local_tm(time_t ts) {
    struct tm tm_res;
    localtime_r(&ts, &tm_res);
    return tm_res;
}

static double compute_avg(const struct RollingStats *stats) {
    return (stats->count == 0) ? 0.0 : stats->sum / (double)stats->count;
}

#ifndef _WIN32
static void signal_handler(int signum) {
    (void)signum;
    g_running = 0;
}
#endif

static size_t count_lines(const char *file_path) {
    FILE *f = fopen(file_path, "r");
    if (!f)
        return 0;

    size_t count = 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '\n')
            ++count;
    }

    fclose(f);
    return count;
}

static void reset_if_oversize(const char *file_path, int type) {
    size_t limit = 0;
    switch (type) {
    case 0:
        limit = MAX_MEASUREMENTS_LOG;
        break;
    case 1:
        limit = MAX_HOURLY_LOG;
        break;
    case 2:
        limit = MAX_DAILY_LOG;
        break;
    default:
        return;
    }

    const size_t lines = count_lines(file_path);
    if (lines >= limit)
        remove(file_path);
}

static void append_line(const char *file_path, const char *msg, int type) {
    reset_if_oversize(file_path, type);

    FILE *f = fopen(file_path, "a");
    if (!f)
        return;

    fprintf(f, "%s\n", msg);
    fclose(f);
}

static void double_to_string(double value, char *out, size_t out_size) {
    /* std::to_string uses 6 digits after the decimal point */
    snprintf(out, out_size, "%.6f", value);
}

static void hash_of_string(const char *src, char *dst, size_t dst_size) {
    size_t len = strlen(src);
    size_t needed = len * 2 + 1;
    if (dst_size < needed) {
        if (dst_size > 0)
            dst[0] = '\0';
        return;
    }

    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)src[i];
        snprintf(dst + i * 2, 3, "%02X", c);
    }
}

static bool parse_packet(const char *line, double *value, char *checksum, size_t checksum_size) {
    if (!line || !value || !checksum || checksum_size == 0)
        return false;

    double v = 0.0;
    char chk_local[128];

    if (sscanf(line, "%lf %127s", &v, chk_local) != 2)
        return false;

    *value = v;
    strncpy(checksum, chk_local, checksum_size - 1);
    checksum[checksum_size - 1] = '\0';
    return true;
}

#ifdef _WIN32
static HANDLE open_serial_port(const char *device) {
    HANDLE hSerial = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hSerial == INVALID_HANDLE_VALUE)
        return INVALID_HANDLE_VALUE;

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 1000;
    timeouts.ReadTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    return hSerial;
}
#else
static int configure_serial(int fd) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0)
        return -1;

    cfmakeraw(&tty);

    if (cfsetispeed(&tty, B115200) != 0 || cfsetospeed(&tty, B115200) != 0)
        return -1;

    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8 | CLOCAL | CREAD;
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
#ifdef CRTSCTS
    tty.c_cflag &= ~CRTSCTS;
#endif

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10; /* 1 second timeout */

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
        return -1;

    return 0;
}

static int open_serial_port(const char *device) {
    int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
        return -1;

    if (configure_serial(fd) != 0) {
        close(fd);
        return -1;
    }

    return fd;
}
#endif

#ifdef _WIN32
static ssize_t read_line_timeout(HANDLE hSerial, char *buf, size_t buf_size, int timeout_sec) {
    (void)timeout_sec; /* timeout set in handle */
    if (buf_size == 0)
        return -1;

    size_t pos = 0;

    while (pos + 1 < buf_size) {
        char ch;
        DWORD bytesRead = 0;

        if (!ReadFile(hSerial, &ch, 1, &bytesRead, NULL)) {
            return -1;
        }

        if (bytesRead == 1) {
            if (ch == '\n' || ch == '\r')
                break;
            buf[pos++] = ch;
        } else {
            break; /* timeout or no data */
        }
    }

    buf[pos] = '\0';
    return (ssize_t)pos;
}
#else
static ssize_t read_line_timeout(int fd, char *buf, size_t buf_size, int timeout_sec) {
    if (buf_size == 0)
        return -1;

    size_t pos = 0;

    while (pos + 1 < buf_size) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        struct timeval tv;
        tv.tv_sec = timeout_sec;
        tv.tv_usec = 0;

        int rv = select(fd + 1, &readfds, NULL, NULL, &tv);
        if (rv == 0) {
            break; /* timeout */
        } else if (rv < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        char ch;
        ssize_t n = read(fd, &ch, 1);
        if (n == 1) {
            if (ch == '\n' || ch == '\r')
                break;
            buf[pos++] = ch;
        } else if (n == 0) {
            break; /* EOF or no data */
        } else {
            if (errno == EINTR)
                continue;
            return -1;
        }
    }

    buf[pos] = '\0';
    return (ssize_t)pos;
}
#endif

static void flush_measurements(struct MeasurementBuffer *buffer) {
    for (size_t i = 0; i < buffer->size; ++i) {
        char line[128];
        snprintf(line, sizeof(line), "%ld %.6f", (long)buffer->items[i].ts, buffer->items[i].value);
        append_line(LOG_MEASURE, line, 0);
    }
    buffer->size = 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        return -1;
    }

#ifndef _WIN32
    /* Устанавливаем обработчики сигналов */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#endif

#ifdef _WIN32
    HANDLE serial_fd = open_serial_port(argv[1]);
    if (serial_fd == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to open port %s\n", argv[1]);
        return -2;
    }
#else
    int serial_fd = open_serial_port(argv[1]);
    if (serial_fd < 0) {
        fprintf(stderr, "Failed to open port %s: %s\n", argv[1], strerror(errno));
        return -2;
    }
#endif

    struct RollingStats hour_stats = {0};
    struct RollingStats day_stats = {0};
    struct MeasurementBuffer buffer = {0};

    struct tm last_tm = local_tm(now_ts());
    int last_hour = last_tm.tm_hour;
    int last_mday = last_tm.tm_mday;

    char line[256];

    printf("Temperature logger started. Press Ctrl+C to stop.\n");

    while (g_running) {
        ssize_t len = read_line_timeout(serial_fd, line, sizeof(line), 1);
        if (len > 0) {
            double value = 0.0;
            char checksum[128];

            if (parse_packet(line, &value, checksum, sizeof(checksum))) {
                bool value_ok = (value >= -60.0 && value <= 60.0);

                if (buffer.size > 0) {
                    double diff = fabs(value - buffer.items[buffer.size - 1].value);
                    value_ok = value_ok && (diff < 1.0);
                }

                if (value_ok) {
                    char value_str[64];
                    double_to_string(value, value_str, sizeof(value_str));

                    char calc[128];
                    hash_of_string(value_str, calc, sizeof(calc));

                    if (strcmp(calc, checksum) == 0) {
                        printf("New value -> %.6f\n", value);
                        time_t ts = now_ts();

                        buffer.items[buffer.size].ts = ts;
                        buffer.items[buffer.size].value = value;
                        buffer.size++;

                        if (buffer.size >= 10) {
                            /* ИСПРАВЛЕНО: Добавляем ВСЕ значения из буфера в статистику */
                            for (size_t i = 0; i < buffer.size; ++i) {
                                hour_stats.sum += buffer.items[i].value;
                                day_stats.sum += buffer.items[i].value;
                            }
                            hour_stats.count += buffer.size;
                            day_stats.count += buffer.size;
                            
                            flush_measurements(&buffer);
                        }
                    }
                }
            }

            struct tm cur_tm = local_tm(now_ts());

            if (cur_tm.tm_hour != last_hour) {
                if (hour_stats.count > 0) {
                    char hourly_line[128];
                    double avg = compute_avg(&hour_stats);
                    snprintf(hourly_line, sizeof(hourly_line), "%ld %.6f", (long)now_ts(), avg);
                    append_line(LOG_HOURLY, hourly_line, 1);
                    printf("Hourly average recorded: %.6f (count: %zu)\n", avg, hour_stats.count);
                }

                hour_stats.sum = 0.0;
                hour_stats.count = 0;
                last_hour = cur_tm.tm_hour;
            }

            if (cur_tm.tm_mday != last_mday) {
                if (day_stats.count > 0) {
                    char daily_line[128];
                    double avg = compute_avg(&day_stats);
                    snprintf(daily_line, sizeof(daily_line), "%ld %.6f", (long)now_ts(), avg);
                    append_line(LOG_DAILY, daily_line, 2);
                    printf("Daily average recorded: %.6f (count: %zu)\n", avg, day_stats.count);
                }

                day_stats.sum = 0.0;
                day_stats.count = 0;
                last_mday = cur_tm.tm_mday;
            }
        }
    }

    /* Финальная запись данных перед завершением */
    printf("\nShutting down...\n");
    
    if (buffer.size > 0) {
        printf("Flushing %zu remaining measurements\n", buffer.size);
        flush_measurements(&buffer);
    }
    
    if (hour_stats.count > 0) {
        char hourly_line[128];
        double avg = compute_avg(&hour_stats);
        snprintf(hourly_line, sizeof(hourly_line), "%ld %.6f", (long)now_ts(), avg);
        append_line(LOG_HOURLY, hourly_line, 1);
        printf("Final hourly average: %.6f (count: %zu)\n", avg, hour_stats.count);
    }
    
    if (day_stats.count > 0) {
        char daily_line[128];
        double avg = compute_avg(&day_stats);
        snprintf(daily_line, sizeof(daily_line), "%ld %.6f", (long)now_ts(), avg);
        append_line(LOG_DAILY, daily_line, 2);
        printf("Final daily average: %.6f (count: %zu)\n", avg, day_stats.count);
    }

#ifdef _WIN32
    CloseHandle(serial_fd);
#else
    close(serial_fd);
#endif
    printf("Temperature logger stopped.\n");
    return 0;
}

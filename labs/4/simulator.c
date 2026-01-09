#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

static void double_to_string(double value, char *out, size_t out_size) {
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
    tty.c_cc[VTIME] = 10;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
        return -1;

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("Failed to open port");
        return -2;
    }

    if (configure_serial(fd) != 0) {
        fprintf(stderr, "Failed to configure serial port\n");
        close(fd);
        return -3;
    }

    srand(time(NULL));
    double base_temp = 20.0;

    printf("Temperature simulator started on %s\n", argv[1]);
    printf("Press Ctrl+C to stop\n");

    for (;;) {
        // Генерируем температуру с небольшими колебаниями
        double variation = ((double)rand() / RAND_MAX) * 2.0 - 1.0; // от -1 до 1
        double temperature = base_temp + variation;

        char value_str[64];
        double_to_string(temperature, value_str, sizeof(value_str));

        char checksum[128];
        hash_of_string(value_str, checksum, sizeof(checksum));

        char packet[256];
        snprintf(packet, sizeof(packet), "%s %s\n", value_str, checksum);

        ssize_t written = write(fd, packet, strlen(packet));
        if (written < 0) {
            perror("Write failed");
            break;
        }

        printf("Sent: %.6f °C\n", temperature);

        // Отправляем данные каждую секунду
        sleep(1);

        // Медленно меняем базовую температуру
        base_temp += ((double)rand() / RAND_MAX) * 0.2 - 0.1;
        if (base_temp < 15.0) base_temp = 15.0;
        if (base_temp > 25.0) base_temp = 25.0;
    }

    close(fd);
    return 0;
}

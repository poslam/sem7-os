#include "shared.h"
#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

int64_t now_ms(void) {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t t = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    return (int64_t)(t / 10000 - 11644473600000LL);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

const char* shared_file_path(void) {
    static char path[512];
    const char* dir = data_dir();
    snprintf(path, sizeof(path), "%s/lab3_shared.bin", dir);
    return path;
}

const char* log_file_path(void) {
    static char path[512];
    const char* dir = data_dir();
    snprintf(path, sizeof(path), "%s/logs/lab3.log", dir);
    return path;
}

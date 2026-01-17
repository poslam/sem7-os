#include "app.h"
#include "shared.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifndef _WIN32
#include <sys/select.h>
#include <unistd.h>
#endif

#define LOG_FILE "lab3/logs/process.log"
#define TIMER_300MS 300
#define TIMER_1SEC 1000
#define TIMER_3SEC 3000

static FILE* g_log_file = NULL;
static run_mode_t g_mode = MODE_NORMAL;
static int g_my_pid = 0;
static bool g_is_master = false;
static long long g_start_time = 0;
static long long g_last_300ms = 0;
static long long g_last_1sec = 0;
static long long g_last_3sec = 0;
static int g_copy1_pid = 0;
static int g_copy2_pid = 0;
static char g_program_path[1024] = {0};
static bool g_running = true;

void app_log(const char* format, ...) {
    if (g_log_file == NULL) return;
    
    char time_buffer[64];
    platform_format_time(time_buffer, sizeof(time_buffer));
    
    fprintf(g_log_file, "[%s] [PID:%d] ", time_buffer, g_my_pid);
    
    va_list args;
    va_start(args, format);
    vfprintf(g_log_file, format, args);
    va_end(args);
    
    fprintf(g_log_file, "\n");
    fflush(g_log_file);
}

int app_init(run_mode_t mode) {
    g_mode = mode;
    g_my_pid = platform_get_pid();
    g_start_time = platform_get_time_ms();
    
    // Инициализация shared memory
    if (shared_init() != 0) {
        fprintf(stderr, "Failed to initialize shared memory\n");
        return -1;
    }
    
    // Открываем лог файл
    g_log_file = platform_open_log(LOG_FILE, true);
    if (g_log_file == NULL) {
        fprintf(stderr, "Failed to open log file\n");
        shared_cleanup();
        return -1;
    }
    
    // Инициализируем таймеры
    g_last_300ms = g_start_time;
    g_last_1sec = g_start_time;
    g_last_3sec = g_start_time;
    
    // Записываем старт в лог
    app_log("Process started (mode=%d)", mode);
    
    // Пробуем стать мастером (только для обычного режима)
    if (g_mode == MODE_NORMAL) {
        g_is_master = shared_try_become_master(g_my_pid);
        if (g_is_master) {
            app_log("Became MASTER process");
            printf("Process %d: MASTER mode\n", g_my_pid);
        } else {
            app_log("Running as SLAVE process (master=%d)", shared_get_master_pid());
            printf("Process %d: SLAVE mode (master=%d)\n", g_my_pid, shared_get_master_pid());
        }
    }
    
    return 0;
}

void app_cleanup(void) {
    app_log("Process exiting");
    
    if (g_log_file != NULL) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    
    shared_cleanup();
}

static void handle_copy1_mode(void) {
    // Копия 1: +10 к счетчику
    app_log("Copy1 started");
    shared_set_copy_status(1, true);
    
    platform_sleep_ms(100); // Небольшая задержка
    
    shared_add_counter(10);
    int64_t counter = shared_get_counter();
    app_log("Copy1: counter += 10, new value = %lld", (long long)counter);
    
    shared_set_copy_status(1, false);
    app_log("Copy1 exiting");
}

static void handle_copy2_mode(void) {
    // Копия 2: *2, ждем 2 сек, /2
    app_log("Copy2 started");
    shared_set_copy_status(2, true);
    
    platform_sleep_ms(100);
    
    shared_multiply_counter(2);
    int64_t counter = shared_get_counter();
    app_log("Copy2: counter *= 2, new value = %lld", (long long)counter);
    
    platform_sleep_ms(2000);
    
    shared_divide_counter(2);
    counter = shared_get_counter();
    app_log("Copy2: counter /= 2, new value = %lld", (long long)counter);
    
    shared_set_copy_status(2, false);
    app_log("Copy2 exiting");
}

static void timer_300ms_handler(void) {
    // Увеличиваем счетчик на 1
    shared_add_counter(1);
}

static void timer_1sec_handler(void) {
    if (!g_is_master) return;
    
    // Логируем текущее состояние
    int64_t counter = shared_get_counter();
    app_log("Timer 1s: counter = %lld", (long long)counter);
}

static void timer_3sec_handler(void) {
    if (!g_is_master) return;
    
    // Проверяем статус предыдущих копий
    bool copy1_busy = shared_get_copy_status(1);
    bool copy2_busy = shared_get_copy_status(2);
    
    // Проверяем реальный статус процессов
    if (g_copy1_pid > 0 && platform_check_process_finished(g_copy1_pid)) {
        g_copy1_pid = 0;
        copy1_busy = false;
        shared_set_copy_status(1, false);
    }
    
    if (g_copy2_pid > 0 && platform_check_process_finished(g_copy2_pid)) {
        g_copy2_pid = 0;
        copy2_busy = false;
        shared_set_copy_status(2, false);
    }
    
    // Запускаем копию 1
    if (!copy1_busy) {
        g_copy1_pid = platform_spawn_copy(g_program_path, MODE_COPY1);
        if (g_copy1_pid > 0) {
            app_log("Spawned Copy1 with PID %d", g_copy1_pid);
        } else {
            app_log("Failed to spawn Copy1");
        }
    } else {
        app_log("Copy1 is still running, skipping spawn");
    }
    
    // Запускаем копию 2
    if (!copy2_busy) {
        g_copy2_pid = platform_spawn_copy(g_program_path, MODE_COPY2);
        if (g_copy2_pid > 0) {
            app_log("Spawned Copy2 with PID %d", g_copy2_pid);
        } else {
            app_log("Failed to spawn Copy2");
        }
    } else {
        app_log("Copy2 is still running, skipping spawn");
    }
}

void app_handle_input(void) {
    // Проверяем наличие данных на stdin (неблокирующий режим)
#ifndef _WIN32
    fd_set readfds;
    struct timeval tv;
    
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
    if (ret <= 0) return; // Нет данных или ошибка
#endif
    
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        // Убираем перевод строки
        buffer[strcspn(buffer, "\n")] = 0;
        
        if (strlen(buffer) == 0) return;
        
        // Парсим команду
        if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0 || strcmp(buffer, "q") == 0) {
            printf("Exiting...\n");
            g_running = false;
        } else if (strcmp(buffer, "get") == 0) {
            int64_t value = shared_get_counter();
            printf("Counter = %lld\n", (long long)value);
        } else if (strncmp(buffer, "set ", 4) == 0) {
            long long value = atoll(buffer + 4);
            shared_set_counter(value);
            printf("Counter set to %lld\n", value);
            app_log("User set counter to %lld", value);
        } else {
            printf("Unknown command. Available: set <value>, get, quit\n");
        }
    }
}

int app_run(void) {
    // Режим копии - выполняем задачу и выходим
    if (g_mode == MODE_COPY1) {
        handle_copy1_mode();
        return 0;
    } else if (g_mode == MODE_COPY2) {
        handle_copy2_mode();
        return 0;
    }
    
    // Обычный режим - бесконечный цикл
    printf("\n=== Lab 3 Process Manager ===\n");
    printf("PID: %d, Mode: %s\n", g_my_pid, g_is_master ? "MASTER" : "SLAVE");
    printf("Commands:\n");
    printf("  set <value> - Set counter value\n");
    printf("  get         - Get current counter value\n");
    printf("  quit        - Exit program\n");
    printf("==============================\n\n");
    
    // Основной цикл
    while (g_running) {
        long long current_time = platform_get_time_ms();
        
        // Таймер 300 мс
        if (current_time - g_last_300ms >= TIMER_300MS) {
            timer_300ms_handler();
            g_last_300ms = current_time;
        }
        
        // Таймер 1 сек
        if (current_time - g_last_1sec >= TIMER_1SEC) {
            timer_1sec_handler();
            g_last_1sec = current_time;
        }
        
        // Таймер 3 сек
        if (current_time - g_last_3sec >= TIMER_3SEC) {
            timer_3sec_handler();
            g_last_3sec = current_time;
        }
        
        // Обработка пользовательского ввода
        app_handle_input();
        
        // Короткая пауза чтобы не нагружать CPU
        platform_sleep_ms(50);
    }
    
    return 0;
}

void app_set_program_path(const char* path) {
    strncpy(g_program_path, path, sizeof(g_program_path) - 1);
    g_program_path[sizeof(g_program_path) - 1] = '\0';
}

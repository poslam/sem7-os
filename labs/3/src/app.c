#include "app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#define sleep_ms(ms) usleep((ms) * 1000)
#endif

#include "platform.h"
#include "shared.h"

typedef struct {
    int mode;
    SharedMap* map;
} ChildThreadData;

typedef struct {
    SharedMap* map;
    volatile int* running;
    int64_t self;
} MainThreadData;

static void run_child(int mode, SharedMap* map) {
    int64_t self = get_pid();
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "%s child%d start pid=%lld", 
             get_timestamp(), mode, (long long)self);
    log_line(log_msg);
    
    {
        char lock_path[600];
        snprintf(lock_path, sizeof(lock_path), "%s.lock", shared_file_path());
        FileLock* lock = file_lock_create(lock_path);
        if (file_lock_is_locked(lock) && map->ptr) {
            if (mode == 1) {
                map->ptr->counter += 10;
            } else {
                map->ptr->counter *= 2;
            }
        }
        file_lock_destroy(lock);
    }
    
    if (mode == 2) {
        sleep_ms(2000);
        char lock_path[600];
        snprintf(lock_path, sizeof(lock_path), "%s.lock", shared_file_path());
        FileLock* lock = file_lock_create(lock_path);
        if (file_lock_is_locked(lock) && map->ptr) {
            map->ptr->counter /= 2;
        }
        file_lock_destroy(lock);
    }
    
    snprintf(log_msg, sizeof(log_msg), "%s child%d exit pid=%lld",
             get_timestamp(), mode, (long long)self);
    log_line(log_msg);
}

static void* input_thread_func(void* arg) {
    MainThreadData* data = (MainThreadData*)arg;
    char line[256];
    
    while (*data->running && fgets(line, sizeof(line), stdin)) {
        char* newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        
        char cmd[64];
        long long val;
        
        if (sscanf(line, "%s %lld", cmd, &val) >= 1) {
            if (strcmp(cmd, "set") == 0) {
                if (sscanf(line, "%s %lld", cmd, &val) == 2) {
                    char lock_path[600];
                    snprintf(lock_path, sizeof(lock_path), "%s.lock", shared_file_path());
                    FileLock* lock = file_lock_create(lock_path);
                    if (file_lock_is_locked(lock) && data->map->ptr) {
                        data->map->ptr->counter = val;
                        char log_msg[256];
                        snprintf(log_msg, sizeof(log_msg), "%s pid=%lld set counter=%lld",
                                get_timestamp(), (long long)data->self, val);
                        log_line(log_msg);
                    }
                    file_lock_destroy(lock);
                } else {
                    printf("Usage: set <number>\n");
                }
            } else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
                *(data->running) = 0;
                break;
            }
        }
    }
    *(data->running) = 0;
    return NULL;
}

static void* counter_thread_func(void* arg) {
    MainThreadData* data = (MainThreadData*)arg;
    
    while (*data->running) {
        sleep_ms(300);
        char lock_path[600];
        snprintf(lock_path, sizeof(lock_path), "%s.lock", shared_file_path());
        FileLock* lock = file_lock_create(lock_path);
        if (file_lock_is_locked(lock) && data->map->ptr) {
            data->map->ptr->counter += 1;
        }
        file_lock_destroy(lock);
    }
    return NULL;
}

void run_app(int is_child, int child_mode) {
    SharedMap map;
    shared_map_init(&map);
    
    if (!map_shared(&map) || !map.ptr) {
        fprintf(stderr, "Failed to map shared state\n");
        return;
    }
    
    int64_t self = get_pid();
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "%s start pid=%lld", get_timestamp(), (long long)self);
    log_line(log_msg);
    
    if (is_child) {
        run_child(child_mode, &map);
        unmap_shared(&map);
        return;
    }
    
    volatile int running = 1;
    pthread_t input_thread, counter_thread;
    
    MainThreadData thread_data;
    thread_data.map = &map;
    thread_data.running = &running;
    thread_data.self = self;
    
    pthread_create(&input_thread, NULL, input_thread_func, &thread_data);
    pthread_create(&counter_thread, NULL, counter_thread_func, &thread_data);
    
    int64_t last_log = now_ms();
    int64_t last_spawn = now_ms();
    
    while (running) {
        sleep_ms(100);
        int64_t t = now_ms();
        
        /* Owner election and heartbeat */
        {
            char lock_path[600];
            snprintf(lock_path, sizeof(lock_path), "%s.lock", shared_file_path());
            FileLock* lock = file_lock_create(lock_path);
            if (file_lock_is_locked(lock) && map.ptr) {
                if (map.ptr->owner_pid == self) {
                    map.ptr->owner_heartbeat_ms = t;
                } else if (should_take_over(map.ptr, self)) {
                    map.ptr->owner_pid = self;
                    map.ptr->owner_heartbeat_ms = t;
                    snprintf(log_msg, sizeof(log_msg), "%s pid=%lld became owner",
                            get_timestamp(), (long long)self);
                    log_line(log_msg);
                }
            }
            file_lock_destroy(lock);
        }
        
        int am_owner = 0;
        {
            char lock_path[600];
            snprintf(lock_path, sizeof(lock_path), "%s.lock", shared_file_path());
            FileLock* lock = file_lock_create(lock_path);
            if (file_lock_is_locked(lock) && map.ptr) {
                am_owner = (map.ptr->owner_pid == self);
            }
            file_lock_destroy(lock);
        }
        
        if (am_owner) {
            if (t - last_log >= 1000) {
                last_log = t;
                int64_t cnt = 0;
                {
                    char lock_path[600];
                    snprintf(lock_path, sizeof(lock_path), "%s.lock", shared_file_path());
                    FileLock* lock = file_lock_create(lock_path);
                    if (file_lock_is_locked(lock) && map.ptr) {
                        cnt = map.ptr->counter;
                    }
                    file_lock_destroy(lock);
                }
                snprintf(log_msg, sizeof(log_msg), "%s pid=%lld counter=%lld",
                        get_timestamp(), (long long)self, (long long)cnt);
                log_line(log_msg);
            }
            
            if (t - last_spawn >= 3000) {
                last_spawn = t;
                int child_running = 0;
                {
                    char lock_path[600];
                    snprintf(lock_path, sizeof(lock_path), "%s.lock", shared_file_path());
                    FileLock* lock = file_lock_create(lock_path);
                    if (file_lock_is_locked(lock) && map.ptr) {
                        if (map.ptr->child1_pid && is_process_alive(map.ptr->child1_pid)) {
                            child_running = 1;
                        } else {
                            map.ptr->child1_pid = 0;
                        }
                        if (map.ptr->child2_pid && is_process_alive(map.ptr->child2_pid)) {
                            child_running = 1;
                        } else {
                            map.ptr->child2_pid = 0;
                        }
                    }
                    file_lock_destroy(lock);
                }
                
                if (child_running) {
                    snprintf(log_msg, sizeof(log_msg), "%s pid=%lld skip spawn: child still running",
                            get_timestamp(), (long long)self);
                    log_line(log_msg);
                } else {
                    int64_t p1 = launch_child(1);
                    int64_t p2 = launch_child(2);
                    {
                        char lock_path[600];
                        snprintf(lock_path, sizeof(lock_path), "%s.lock", shared_file_path());
                        FileLock* lock = file_lock_create(lock_path);
                        if (file_lock_is_locked(lock) && map.ptr) {
                            map.ptr->child1_pid = p1;
                            map.ptr->child2_pid = p2;
                        }
                        file_lock_destroy(lock);
                    }
                    if (!p1 || !p2) {
                        snprintf(log_msg, sizeof(log_msg), "%s pid=%lld failed to spawn child(s)",
                                get_timestamp(), (long long)self);
                        log_line(log_msg);
                    } else {
                        snprintf(log_msg, sizeof(log_msg), "%s pid=%lld spawned children %lld, %lld",
                                get_timestamp(), (long long)self, (long long)p1, (long long)p2);
                        log_line(log_msg);
                    }
                }
            }
        }
    }
    
    running = 0;
    pthread_join(input_thread, NULL);
    pthread_join(counter_thread, NULL);
    
    snprintf(log_msg, sizeof(log_msg), "%s exit pid=%lld", get_timestamp(), (long long)self);
    log_line(log_msg);
    unmap_shared(&map);
}

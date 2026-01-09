#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include "shared.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int64_t get_pid(void);
    int is_process_alive(int64_t other_pid);
    const char *get_timestamp(void);
    const char *exe_path(void);
    const char *data_dir(void);

    typedef struct
    {
#ifdef _WIN32
        void *handle; /* HANDLE */
#else
    int fd;
#endif
    } FileLock;

    FileLock *file_lock_create(const char *path);
    void file_lock_destroy(FileLock *lock);
    int file_lock_is_locked(const FileLock *lock);

    void log_line(const char *line);

    typedef struct
    {
#ifdef _WIN32
        void *file;    /* HANDLE */
        void *mapping; /* HANDLE */
#else
    int fd;
#endif
        SharedState *ptr;
    } SharedMap;

    void shared_map_init(SharedMap *m);
    int map_shared(SharedMap *m);
    void unmap_shared(SharedMap *m);
    int should_take_over(const SharedState *s, int64_t self);
    int64_t launch_child(int mode);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_H */

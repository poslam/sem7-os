#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        int64_t counter;
        int64_t owner_pid;
        int64_t owner_heartbeat_ms;
        int64_t child1_pid;
        int64_t child2_pid;
        int64_t child1_start_ms;
        int64_t child2_start_ms;
    } SharedState;

    int64_t now_ms(void);
    const char *shared_file_path(void);
    const char *log_file_path(void);

#ifdef __cplusplus
}
#endif

#endif /* SHARED_H */

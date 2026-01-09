#include "bg_process.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    int timeout_ms = -1;
    int first_cmd_arg = 1;

    if (argc > 2 && strcmp(argv[1], "--timeout") == 0) {
        timeout_ms = atoi(argv[2]);
        if (timeout_ms == 0 && strcmp(argv[2], "0") != 0) {
            fprintf(stderr, "Invalid timeout value: %s\n", argv[2]);
            return 1;
        }
        first_cmd_arg = 3;
    }

    if (argc <= first_cmd_arg) {
        fprintf(stderr, "Usage: %s [--timeout <ms>] <program> [args...]\n", argv[0]);
        return 1;
    }

    int args_count = argc - first_cmd_arg;
    const char** args = (const char**)&argv[first_cmd_arg];

    ProcessHandle handle;
    bgproc_init_handle(&handle);
    
    char err[512];
    if (bgproc_start(args, args_count, &handle, err, sizeof(err)) != 0) {
        fprintf(stderr, "Failed to start process: %s\n", err);
        return 1;
    }

    printf("Started:");
    for (int i = 0; i < args_count; ++i) {
        printf(" %s", args[i]);
    }
    printf("\n");

    int exit_code = 0;
    if (bgproc_wait(&handle, &exit_code, timeout_ms, err, sizeof(err)) != 0) {
        bgproc_close(&handle);
        fprintf(stderr, "Wait failed: %s\n", err);
        return 1;
    }

    printf("Process exited with code %d\n", exit_code);
    return exit_code;
}

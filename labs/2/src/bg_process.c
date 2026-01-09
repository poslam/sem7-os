#include "bg_process.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#endif

static void set_err(char* buf, int size, const char* msg) {
    if (buf && size > 0) {
        strncpy(buf, msg, size - 1);
        buf[size - 1] = '\0';
    }
}

#ifdef _WIN32

static char* quote_arg(const char* arg) {
    if (!arg || !*arg) {
        char* result = malloc(3);
        strcpy(result, "\"\"");
        return result;
    }

    int need_quotes = (strpbrk(arg, " \t\"") != NULL);
    if (!need_quotes) {
        return strdup(arg);
    }

    size_t len = strlen(arg);
    char* result = malloc(len * 2 + 3);
    char* p = result;
    
    *p++ = '"';
    
    unsigned backslashes = 0;
    for (const char* s = arg; *s; ++s) {
        if (*s == '\\') {
            ++backslashes;
            continue;
        }
        
        if (*s == '"') {
            for (unsigned i = 0; i < backslashes * 2 + 1; ++i) {
                *p++ = '\\';
            }
            *p++ = '"';
            backslashes = 0;
            continue;
        }
        
        for (unsigned i = 0; i < backslashes; ++i) {
            *p++ = '\\';
        }
        backslashes = 0;
        *p++ = *s;
    }
    
    for (unsigned i = 0; i < backslashes; ++i) {
        *p++ = '\\';
    }
    *p++ = '"';
    *p = '\0';
    
    return result;
}

static char* make_command_line(const char* const* args, int args_count) {
    size_t total_len = 0;
    char** quoted = malloc(args_count * sizeof(char*));
    
    for (int i = 0; i < args_count; ++i) {
        quoted[i] = quote_arg(args[i]);
        total_len += strlen(quoted[i]);
        if (i > 0) total_len++;  /* space */
    }
    
    char* result = malloc(total_len + 1);
    char* p = result;
    
    for (int i = 0; i < args_count; ++i) {
        if (i > 0) *p++ = ' ';
        strcpy(p, quoted[i]);
        p += strlen(quoted[i]);
        free(quoted[i]);
    }
    
    free(quoted);
    return result;
}

static void format_last_error(char* buf, int size) {
    DWORD code = GetLastError();
    if (code == 0) {
        buf[0] = '\0';
        return;
    }

    char temp[256];
    snprintf(temp, sizeof(temp), "Win32 error %lu", code);

    LPSTR buffer = NULL;
    DWORD msg_size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buffer,
        0,
        NULL);

    if (msg_size && buffer) {
        snprintf(buf, size, "%s: %s", temp, buffer);
        LocalFree(buffer);
    } else {
        strncpy(buf, temp, size - 1);
        buf[size - 1] = '\0';
    }
}

#endif

void bgproc_init_handle(ProcessHandle* handle) {
#ifdef _WIN32
    handle->process = NULL;
#else
    handle->pid = -1;
#endif
}

int bgproc_start(const char* const* args, int args_count, ProcessHandle* handle,
                 char* err_buf, int err_size) {
    if (args_count <= 0 || !args || !args[0]) {
        set_err(err_buf, err_size, "No executable specified");
        return -1;
    }

    bgproc_close(handle);

#ifdef _WIN32
    char* cmdline = make_command_line(args, args_count);
    
    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    
    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    BOOL ok = CreateProcessA(
        NULL,
        cmdline,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi);

    free(cmdline);

    if (!ok) {
        if (err_buf && err_size > 0) {
            format_last_error(err_buf, err_size);
        }
        return -1;
    }

    CloseHandle(pi.hThread);
    handle->process = pi.hProcess;
    return 0;
#else
    pid_t pid = fork();
    if (pid == 0) {
        /* child process */
        char** argv = malloc((args_count + 1) * sizeof(char*));
        for (int i = 0; i < args_count; ++i) {
            argv[i] = (char*)args[i];
        }
        argv[args_count] = NULL;
        
        execvp(argv[0], argv);
        _exit(127);  /* execvp failed */
    }

    if (pid < 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "fork failed: %d", errno);
        set_err(err_buf, err_size, msg);
        return -1;
    }

    handle->pid = (int)pid;
    return 0;
#endif
}

int bgproc_wait(ProcessHandle* handle, int* exit_code, int timeout_ms,
                char* err_buf, int err_size) {
#ifdef _WIN32
    if (!handle->process) {
        set_err(err_buf, err_size, "Process handle is empty");
        return -1;
    }

    DWORD wait_time = timeout_ms < 0 ? INFINITE : (DWORD)timeout_ms;
    DWORD res = WaitForSingleObject(handle->process, wait_time);

    if (res == WAIT_TIMEOUT) {
        set_err(err_buf, err_size, "Timed out");
        return -1;
    }

    if (res != WAIT_OBJECT_0) {
        if (err_buf && err_size > 0) {
            format_last_error(err_buf, err_size);
        }
        return -1;
    }

    DWORD code = 0;
    if (!GetExitCodeProcess(handle->process, &code)) {
        if (err_buf && err_size > 0) {
            format_last_error(err_buf, err_size);
        }
        return -1;
    }

    CloseHandle(handle->process);
    handle->process = NULL;
    *exit_code = (int)code;
    return 0;
#else
    if (handle->pid <= 0) {
        set_err(err_buf, err_size, "PID is empty");
        return -1;
    }

    int status = 0;
    if (timeout_ms < 0) {
        while (1) {
            pid_t r = waitpid(handle->pid, &status, 0);
            if (r == -1 && errno == EINTR) {
                continue;
            } else if (r == handle->pid) {
                break;
            } else {
                char msg[256];
                snprintf(msg, sizeof(msg), "waitpid failed: %d", errno);
                set_err(err_buf, err_size, msg);
                return -1;
            }
        }
    } else {
        struct timespec start, now;
        clock_gettime(CLOCK_MONOTONIC, &start);
        long long deadline = (long long)start.tv_sec * 1000 + start.tv_nsec / 1000000 + timeout_ms;
        
        while (1) {
            pid_t r = waitpid(handle->pid, &status, WNOHANG);
            if (r == 0) {
                clock_gettime(CLOCK_MONOTONIC, &now);
                long long current = (long long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
                if (current >= deadline) {
                    set_err(err_buf, err_size, "Timed out");
                    return -1;
                }
                
                struct timespec sleep_time;
                sleep_time.tv_sec = 0;
                sleep_time.tv_nsec = 25000000;  /* 25ms */
                nanosleep(&sleep_time, NULL);
                continue;
            } else if (r == -1) {
                if (errno == EINTR) {
                    continue;
                }
                char msg[256];
                snprintf(msg, sizeof(msg), "waitpid failed: %d", errno);
                set_err(err_buf, err_size, msg);
                return -1;
            } else {
                break;
            }
        }
    }

    handle->pid = -1;
    if (WIFEXITED(status)) {
        *exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        *exit_code = 128 + WTERMSIG(status);
    } else {
        *exit_code = status;
    }
    return 0;
#endif
}

void bgproc_close(ProcessHandle* handle) {
#ifdef _WIN32
    if (handle->process) {
        CloseHandle(handle->process);
        handle->process = NULL;
    }
#else
    handle->pid = -1;
#endif
}

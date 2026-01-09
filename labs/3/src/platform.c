#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#endif

static void ensure_dir_exists(const char* path) {
    char temp[512];
    char* p;
    snprintf(temp, sizeof(temp), "%s", path);
    
    for (p = temp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            mkdir(temp, 0755);
            *p = '/';
        }
    }
    mkdir(temp, 0755);
}

int64_t get_pid(void) {
#ifdef _WIN32
    return (int64_t)GetCurrentProcessId();
#else
    return (int64_t)getpid();
#endif
}

int is_process_alive(int64_t other_pid) {
    if (other_pid <= 0) return 0;
#ifdef _WIN32
    HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)other_pid);
    if (!h) return 0;
    DWORD code = WaitForSingleObject(h, 0);
    CloseHandle(h);
    return code == WAIT_TIMEOUT;
#else
    int r = kill((pid_t)other_pid, 0);
    return r == 0 || errno == EPERM;
#endif
}

const char* get_timestamp(void) {
    static char buf[64];
    time_t now = time(NULL);
    struct tm tm_info;
#ifdef _WIN32
    localtime_s(&tm_info, &now);
#else
    localtime_r(&now, &tm_info);
#endif
    
    int64_t ms = now_ms() % 1000;
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03lld",
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
             tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec,
             (long long)ms);
    return buf;
}

const char* exe_path(void) {
    static char path[512];
#ifdef _WIN32
    GetModuleFileNameA(NULL, path, sizeof(path));
#else
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len > 0) {
        path[len] = '\0';
    } else {
        path[0] = '\0';
    }
#endif
    return path;
}

const char* data_dir(void) {
    static char dir[512];
    static int initialized = 0;
    
    if (initialized) return dir;
    
    const char* exe = exe_path();
    if (exe && *exe) {
        char temp[512];
        snprintf(temp, sizeof(temp), "%s", exe);
#ifdef _WIN32
        char* last_slash = strrchr(temp, '\\');
        if (!last_slash) last_slash = strrchr(temp, '/');
#else
        char* last_slash = strrchr(temp, '/');
#endif
        if (last_slash) {
            *last_slash = '\0';
            last_slash = strrchr(temp, '/');
            if (!last_slash) last_slash = strrchr(temp, '\\');
            if (last_slash) {
                *last_slash = '\0';
                last_slash = strrchr(temp, '/');
                if (!last_slash) last_slash = strrchr(temp, '\\');
                if (last_slash) *last_slash = '\0';
            }
        }
        snprintf(dir, sizeof(dir), "%s", temp);
    }
    
    if (!dir[0]) {
        snprintf(dir, sizeof(dir), "lab3");
    }
    
    ensure_dir_exists(dir);
    initialized = 1;
    return dir;
}

FileLock* file_lock_create(const char* path) {
    FileLock* lock = malloc(sizeof(FileLock));
    if (!lock) return NULL;
    
    char parent[512];
    snprintf(parent, sizeof(parent), "%s", path);
    char* last_slash = strrchr(parent, '/');
    if (!last_slash) last_slash = strrchr(parent, '\\');
    if (last_slash) {
        *last_slash = '\0';
        ensure_dir_exists(parent);
    }
    
#ifdef _WIN32
    lock->handle = CreateFileA(path, GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (lock->handle == INVALID_HANDLE_VALUE) {
        free(lock);
        return NULL;
    }
    OVERLAPPED ov = {0};
    if (!LockFileEx(lock->handle, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ov)) {
        CloseHandle(lock->handle);
        free(lock);
        return NULL;
    }
#else
    lock->fd = open(path, O_CREAT | O_RDWR, 0666);
    if (lock->fd < 0) {
        free(lock);
        return NULL;
    }
    struct flock fl = {0};
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    if (fcntl(lock->fd, F_SETLKW, &fl) == -1) {
        close(lock->fd);
        free(lock);
        return NULL;
    }
#endif
    return lock;
}

int file_lock_is_locked(const FileLock* lock) {
    if (!lock) return 0;
#ifdef _WIN32
    return lock->handle != NULL && lock->handle != INVALID_HANDLE_VALUE;
#else
    return lock->fd >= 0;
#endif
}

void file_lock_destroy(FileLock* lock) {
    if (!lock) return;
#ifdef _WIN32
    if (lock->handle && lock->handle != INVALID_HANDLE_VALUE) {
        OVERLAPPED ov = {0};
        UnlockFileEx(lock->handle, 0, MAXDWORD, MAXDWORD, &ov);
        CloseHandle(lock->handle);
    }
#else
    if (lock->fd >= 0) {
        struct flock fl = {0};
        fl.l_type = F_UNLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;
        fcntl(lock->fd, F_SETLKW, &fl);
        close(lock->fd);
    }
#endif
    free(lock);
}

void log_line(const char* line) {
    const char* path = log_file_path();
    char lock_path[600];
    snprintf(lock_path, sizeof(lock_path), "%s.lock", path);
    
    char parent[512];
    snprintf(parent, sizeof(parent), "%s", path);
    char* last_slash = strrchr(parent, '/');
    if (!last_slash) last_slash = strrchr(parent, '\\');
    if (last_slash) {
        *last_slash = '\0';
        ensure_dir_exists(parent);
    }
    
    FileLock* lock = file_lock_create(lock_path);
    FILE* f = fopen(path, "a");
    if (f) {
        fprintf(f, "%s\n", line);
        fclose(f);
    }
    file_lock_destroy(lock);
}

void shared_map_init(SharedMap* m) {
#ifdef _WIN32
    m->file = NULL;
    m->mapping = NULL;
#else
    m->fd = -1;
#endif
    m->ptr = NULL;
}

int map_shared(SharedMap* m) {
    const char* path = shared_file_path();
    
    char parent[512];
    snprintf(parent, sizeof(parent), "%s", path);
    char* last_slash = strrchr(parent, '/');
    if (!last_slash) last_slash = strrchr(parent, '\\');
    if (last_slash) {
        *last_slash = '\0';
        ensure_dir_exists(parent);
    }
    
#ifdef _WIN32
    HANDLE file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    
    LARGE_INTEGER size;
    size.QuadPart = sizeof(SharedState);
    if (!SetFilePointerEx(file, size, NULL, FILE_BEGIN) || !SetEndOfFile(file)) {
        CloseHandle(file);
        return 0;
    }
    
    HANDLE mapping = CreateFileMappingA(file, NULL, PAGE_READWRITE, 0, sizeof(SharedState), NULL);
    if (!mapping) {
        CloseHandle(file);
        return 0;
    }
    
    void* ptr = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedState));
    if (!ptr) {
        CloseHandle(mapping);
        CloseHandle(file);
        return 0;
    }
    
    m->file = file;
    m->mapping = mapping;
    m->ptr = (SharedState*)ptr;
    return 1;
#else
    int fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) return 0;
    
    if (ftruncate(fd, sizeof(SharedState)) != 0) {
        close(fd);
        return 0;
    }
    
    void* p = mmap(NULL, sizeof(SharedState), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        close(fd);
        return 0;
    }
    
    m->fd = fd;
    m->ptr = (SharedState*)p;
    return 1;
#endif
}

void unmap_shared(SharedMap* m) {
#ifdef _WIN32
    if (m->ptr) UnmapViewOfFile(m->ptr);
    if (m->mapping) CloseHandle(m->mapping);
    if (m->file) CloseHandle(m->file);
    m->file = NULL;
    m->mapping = NULL;
#else
    if (m->ptr) munmap(m->ptr, sizeof(SharedState));
    if (m->fd >= 0) close(m->fd);
    m->fd = -1;
#endif
    m->ptr = NULL;
}

int should_take_over(const SharedState* s, int64_t self) {
    int64_t now = now_ms();
    int heartbeat_stale = (now - s->owner_heartbeat_ms) > 4000;
    int owner_dead = !is_process_alive(s->owner_pid);
    return s->owner_pid == 0 || owner_dead || heartbeat_stale;
}

int64_t launch_child(int mode) {
    const char* exe = exe_path();
    if (!exe || !*exe) return 0;
    
#ifdef _WIN32
    char cmdline[1024];
    snprintf(cmdline, sizeof(cmdline), "\"%s\" --child=%d", exe, mode);
    
    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};
    
    BOOL ok = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (!ok) return 0;
    
    int64_t child_pid = (int64_t)pi.dwProcessId;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return child_pid;
#else
    pid_t cpid = fork();
    if (cpid == 0) {
        if (mode == 1) {
            execl(exe, exe, "--child=1", (char*)NULL);
        } else {
            execl(exe, exe, "--child=2", (char*)NULL);
        }
        _exit(127);
    }
    return cpid > 0 ? (int64_t)cpid : 0;
#endif
}

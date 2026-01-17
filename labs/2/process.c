#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/wait.h>
    #include <errno.h>
    #include <signal.h>
#endif

process_handle_t start_background_process(const char* program_path, char* const args[]) {
    if (program_path == NULL) {
        fprintf(stderr, "Error: program_path is NULL\n");
#ifdef _WIN32
        return INVALID_HANDLE_VALUE;
#else
        return -1;
#endif
    }

#ifdef _WIN32
    // Windows implementation
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    // Формируем командную строку
    char cmdline[4096] = {0};
    snprintf(cmdline, sizeof(cmdline), "\"%s\"", program_path);
    
    if (args != NULL) {
        for (int i = 0; args[i] != NULL; i++) {
            strncat(cmdline, " ", sizeof(cmdline) - strlen(cmdline) - 1);
            strncat(cmdline, args[i], sizeof(cmdline) - strlen(cmdline) - 1);
        }
    }
    
    printf("Starting process: %s\n", cmdline);
    
    if (!CreateProcessA(
        NULL,           // Имя модуля
        cmdline,        // Командная строка
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi)            // Pointer to PROCESS_INFORMATION structure
    ) {
        fprintf(stderr, "CreateProcess failed (%d).\n", GetLastError());
        return INVALID_HANDLE_VALUE;
    }
    
    // Закрываем дескриптор потока, он нам не нужен
    CloseHandle(pi.hThread);
    
    return pi.hProcess;
    
#else
    // POSIX implementation
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        return -1;
    }
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Child process starting: %s\n", program_path);
        
        // Формируем массив аргументов для execv
        int arg_count = 1; // program_path
        if (args != NULL) {
            while (args[arg_count - 1] != NULL) {
                arg_count++;
            }
        }
        
        char** exec_args = (char**)malloc((arg_count + 1) * sizeof(char*));
        exec_args[0] = (char*)program_path;
        
        if (args != NULL) {
            for (int i = 0; args[i] != NULL; i++) {
                exec_args[i + 1] = args[i];
            }
        }
        exec_args[arg_count] = NULL;
        
        execv(program_path, exec_args);
        
        // Если execv вернулся, значит произошла ошибка
        perror("execv failed");
        free(exec_args);
        exit(1);
    }
    
    // Родительский процесс
    printf("Started background process with PID: %d\n", pid);
    return pid;
#endif
}

int wait_for_process(process_handle_t handle, int* exit_code) {
#ifdef _WIN32
    if (handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error: invalid process handle\n");
        return -1;
    }
    
    DWORD result = WaitForSingleObject(handle, INFINITE);
    
    if (result == WAIT_FAILED) {
        fprintf(stderr, "WaitForSingleObject failed (%d)\n", GetLastError());
        return -1;
    }
    
    if (exit_code != NULL) {
        DWORD exit_code_dword;
        if (GetExitCodeProcess(handle, &exit_code_dword)) {
            *exit_code = (int)exit_code_dword;
        } else {
            fprintf(stderr, "GetExitCodeProcess failed (%d)\n", GetLastError());
            *exit_code = -1;
        }
    }
    
    return 0;
    
#else
    if (handle < 0) {
        fprintf(stderr, "Error: invalid process handle\n");
        return -1;
    }
    
    int status;
    pid_t result = waitpid(handle, &status, 0);
    
    if (result < 0) {
        if (errno == ECHILD) {
            // Процесс уже был обработан, попробуем получить статус через kill
            if (kill(handle, 0) != 0 && errno == ESRCH) {
                // Процесс действительно завершен, но статус уже был получен
                if (exit_code != NULL) {
                    *exit_code = 0; // Неизвестный статус
                }
                return 0;
            }
        }
        perror("waitpid failed");
        return -1;
    }
    
    if (exit_code != NULL) {
        if (WIFEXITED(status)) {
            *exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            *exit_code = -1;
        } else {
            *exit_code = -1;
        }
    }
    
    return 0;
#endif
}

int is_process_running(process_handle_t handle) {
#ifdef _WIN32
    if (handle == INVALID_HANDLE_VALUE) {
        return -1;
    }
    
    DWORD exit_code;
    if (!GetExitCodeProcess(handle, &exit_code)) {
        return -1;
    }
    
    return (exit_code == STILL_ACTIVE) ? 1 : 0;
    
#else
    if (handle < 0) {
        return -1;
    }
    
    // Используем kill(pid, 0) для проверки существования процесса
    // без забирания его статуса завершения
    if (kill(handle, 0) == 0) {
        // Процесс существует
        return 1;
    }
    
    if (errno == ESRCH) {
        // Процесс не существует (завершился)
        return 0;
    }
    
    // Другая ошибка (например, нет прав)
    return -1;
#endif
}

void close_process_handle(process_handle_t handle) {
#ifdef _WIN32
    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
    }
#else
    // В POSIX нет необходимости закрывать PID
    (void)handle;
#endif
}

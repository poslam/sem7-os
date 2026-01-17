#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep_ms(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define sleep_ms(ms) usleep((ms) * 1000)
#endif

void test_basic_execution() {
    printf("\n=== Test 1: Basic process execution ===\n");
    
    char* args[] = { NULL };
    process_handle_t handle = start_background_process("./test_program", args);
    
#ifdef _WIN32
    if (handle == INVALID_HANDLE_VALUE) {
#else
    if (handle < 0) {
#endif
        fprintf(stderr, "Failed to start process\n");
        return;
    }
    
    printf("Process started, waiting for completion...\n");
    
    int exit_code;
    if (wait_for_process(handle, &exit_code) == 0) {
        printf("Process completed with exit code: %d\n", exit_code);
    } else {
        fprintf(stderr, "Failed to wait for process\n");
    }
    
    close_process_handle(handle);
}

void test_with_arguments() {
    printf("\n=== Test 2: Process with arguments ===\n");
    
    char* args[] = { "--sleep", "1000", "--exit-code", "42", NULL };
    process_handle_t handle = start_background_process("./test_program", args);
    
#ifdef _WIN32
    if (handle == INVALID_HANDLE_VALUE) {
#else
    if (handle < 0) {
#endif
        fprintf(stderr, "Failed to start process\n");
        return;
    }
    
    printf("Process started with custom arguments\n");
    
    int exit_code;
    if (wait_for_process(handle, &exit_code) == 0) {
        printf("Process completed with exit code: %d\n", exit_code);
        if (exit_code == 42) {
            printf("✓ Exit code is correct!\n");
        } else {
            printf("✗ Expected exit code 42, got %d\n", exit_code);
        }
    }
    
    close_process_handle(handle);
}

void test_process_status() {
    printf("\n=== Test 3: Check process status ===\n");
    
    char* args[] = { "--sleep", "3000", NULL };
    process_handle_t handle = start_background_process("./test_program", args);
    
#ifdef _WIN32
    if (handle == INVALID_HANDLE_VALUE) {
#else
    if (handle < 0) {
#endif
        fprintf(stderr, "Failed to start process\n");
        return;
    }
    
    printf("Process started, checking status...\n");
    
    // Проверяем статус несколько раз
    for (int i = 0; i < 5; i++) {
        sleep_ms(800);
        int status = is_process_running(handle);
        if (status == 1) {
            printf("  [%d] Process is still running\n", i + 1);
        } else if (status == 0) {
            printf("  [%d] Process has finished\n", i + 1);
            break;
        } else {
            printf("  [%d] Error checking process status\n", i + 1);
        }
    }
    
    int exit_code;
    wait_for_process(handle, &exit_code);
    printf("Process completed with exit code: %d\n", exit_code);
    
    close_process_handle(handle);
}

void test_multiple_processes() {
    printf("\n=== Test 4: Multiple processes ===\n");
    
    const int num_processes = 3;
    process_handle_t handles[3];
    
    char* args1[] = { "--sleep", "1000", "--exit-code", "1", NULL };
    char* args2[] = { "--sleep", "2000", "--exit-code", "2", NULL };
    char* args3[] = { "--sleep", "1500", "--exit-code", "3", NULL };
    
    printf("Starting %d processes...\n", num_processes);
    
    handles[0] = start_background_process("./test_program", args1);
    handles[1] = start_background_process("./test_program", args2);
    handles[2] = start_background_process("./test_program", args3);
    
    printf("All processes started, waiting for completion...\n");
    
    for (int i = 0; i < num_processes; i++) {
#ifdef _WIN32
        if (handles[i] == INVALID_HANDLE_VALUE) continue;
#else
        if (handles[i] < 0) continue;
#endif
        
        int exit_code;
        if (wait_for_process(handles[i], &exit_code) == 0) {
            printf("Process %d completed with exit code: %d\n", i + 1, exit_code);
        }
        close_process_handle(handles[i]);
    }
    
    printf("All processes finished\n");
}

void test_error_handling() {
    printf("\n=== Test 5: Error handling ===\n");
    
    printf("Trying to start non-existent program...\n");
    char* args[] = { NULL };
    process_handle_t handle = start_background_process("./non_existent_program", args);
    
#ifdef _WIN32
    if (handle == INVALID_HANDLE_VALUE) {
        printf("✓ Correctly handled error for non-existent program (Windows)\n");
    } else {
        printf("✗ Should have failed to start non-existent program\n");
        close_process_handle(handle);
    }
#else
    // На POSIX fork() успешен, но execv в дочернем процессе падает
    // Проверим, что дочерний процесс быстро завершается с ошибкой
    if (handle < 0) {
        printf("✗ Fork failed (unexpected)\n");
    } else {
        printf("Note: On POSIX, fork succeeds but child will fail on execv\n");
        sleep_ms(100); // Даем время дочернему процессу упасть
        
        int exit_code;
        if (wait_for_process(handle, &exit_code) == 0) {
            if (exit_code != 0) {
                printf("✓ Child process failed with exit code: %d\n", exit_code);
            } else {
                printf("✗ Child should have failed but returned 0\n");
            }
        }
        close_process_handle(handle);
    }
#endif
}

int main() {
    printf("========================================\n");
    
    test_basic_execution();
    test_with_arguments();
    test_process_status();
    test_multiple_processes();
    test_error_handling();
    
    printf("\n========================================\n");
    
    return 0;
}

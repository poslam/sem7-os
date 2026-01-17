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

int main(int argc, char* argv[]) {
    printf("Test program started with %d arguments\n", argc);
    
    for (int i = 0; i < argc; i++) {
        printf("  arg[%d] = %s\n", i, argv[i]);
    }
    
    int exit_code = 0;
    int sleep_time = 2000;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--exit-code") == 0 && i + 1 < argc) {
            exit_code = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--sleep") == 0 && i + 1 < argc) {
            sleep_time = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--error") == 0) {
            fprintf(stderr, "This is an error message\n");
        }
    }
    
    printf("Test program will sleep for %d ms and exit with code %d\n", 
           sleep_time, exit_code);
    
    for (int i = 0; i < sleep_time / 500; i++) {
        printf(".");
        fflush(stdout);
        sleep_ms(500);
    }
    printf("\n");
    
    printf("Test program finished\n");
    return exit_code;
}

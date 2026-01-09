#include "app.h"

#include <string.h>

int main(int argc, char* argv[]) {
    int is_child = 0;
    int child_mode = 0;
    
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--child=", 8) == 0) {
            is_child = 1;
            child_mode = (argv[i][8] == '1') ? 1 : 2;
        }
    }
    
    run_app(is_child, child_mode);
    return 0;
}

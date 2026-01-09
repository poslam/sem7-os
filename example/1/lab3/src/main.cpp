#include "app.h"

#include <string>

int main(int argc, char* argv[]) {
    bool is_child = false;
    int child_mode = 0;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--child=", 0) == 0) {
            is_child = true;
            child_mode = (a.back() == '1') ? 1 : 2;
        }
    }

    lab3::run_app(is_child, child_mode);
    return 0;
}

#include "bg_process.h"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    int timeout_ms = -1;
    int first_cmd_arg = 1;

    if (argc > 2 && std::string(argv[1]) == "--timeout") {
        try {
            timeout_ms = std::stoi(argv[2]);
        } catch (const std::exception&) {
            std::cerr << "Invalid timeout value: " << argv[2] << "\n";
            return 1;
        }
        first_cmd_arg = 3;
    }

    if (argc <= first_cmd_arg) {
        std::cerr << "Usage: " << argv[0] << " [--timeout <ms>] <program> [args...]\n";
        return 1;
    }

    std::vector<std::string> args;
    for (int i = first_cmd_arg; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    bgproc::ProcessHandle handle;
    std::string err;

    if (!bgproc::start(args, handle, &err)) {
        std::cerr << "Failed to start process: " << err << "\n";
        return 1;
    }

    std::cout << "Started: ";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) std::cout << ' ';
        std::cout << args[i];
    }
    std::cout << "\n";

    int exit_code = 0;
    if (!bgproc::wait(handle, exit_code, timeout_ms, &err)) {
        bgproc::close(handle);
        std::cerr << "Wait failed: " << err << "\n";
        return 1;
    }

    std::cout << "Process exited with code " << exit_code << "\n";
    return exit_code;
}

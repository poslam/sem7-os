#include "backend.h"

#include <atomic>
#include <thread>

namespace lab5 {
int run_main(bool simulate);
void request_stop();
}  // namespace lab5

namespace {
std::thread backend_thread;
std::atomic<bool> backend_running{false};
}

void start_backend(bool simulate) {
    if (backend_running.load()) return;
    backend_running = true;
    backend_thread = std::thread([simulate]() {
        lab5::run_main(simulate);
        backend_running = false;
    });
}

void stop_backend() {
    if (backend_thread.joinable()) {
        if (backend_running.load()) {
            lab5::request_stop();
        }
        backend_thread.join();
        backend_running = false;
    }
}

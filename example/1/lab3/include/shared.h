#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace lab3 {

inline std::int64_t now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

struct SharedState {
    std::int64_t counter = 0;
    std::int64_t owner_pid = 0;
    std::int64_t owner_heartbeat_ms = 0;
    std::int64_t child1_pid = 0;
    std::int64_t child2_pid = 0;
    std::int64_t child1_start_ms = 0;
    std::int64_t child2_start_ms = 0;
};

// Full paths for shared/log files (inside lab3; log stored in subfolder logs).
std::string shared_file_path();
std::string log_file_path();

}  // namespace lab3

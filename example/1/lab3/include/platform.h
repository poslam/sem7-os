#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "shared.h"

namespace lab3 {

std::int64_t pid();
bool is_process_alive(std::int64_t other_pid);
std::string timestamp();
std::string exe_path();

// Каталог для данных (lab3), общий для логов и shared-файла.
std::string data_dir();

class FileLock {
public:
    explicit FileLock(const std::string& path);
    ~FileLock();
    bool locked() const;
private:
#ifdef _WIN32
    void* handle_ = nullptr;  // HANDLE
#else
    int fd_ = -1;
#endif
};

void log_line(const std::string& line);

struct SharedMap {
#ifdef _WIN32
    void* file = nullptr;     // HANDLE
    void* mapping = nullptr;  // HANDLE
#else
    int fd = -1;
#endif
    SharedState* ptr = nullptr;
};

bool map_shared(SharedMap& m);
void unmap_shared(SharedMap& m);
bool should_take_over(const SharedState& s, std::int64_t self);
std::int64_t launch_child(int mode);

}  // namespace lab3

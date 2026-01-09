#include "common.h"

#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace lab5 {
namespace {
std::string exe_path() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::string(buffer, buffer + len);
#else
    char buffer[4096];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len <= 0) return {};
    buffer[len] = '\0';
    return std::string(buffer);
#endif
}
}

std::string iso_time(const TimePoint& tp) {
    using namespace std::chrono;
    auto tt = Clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

static std::string cached_dir;

std::string data_dir() {
    if (!cached_dir.empty()) return cached_dir;
    fs::path base;
    const auto exe = exe_path();
    if (!exe.empty()) {
        fs::path p(exe);
        if (p.has_parent_path()) p = p.parent_path();  // bin
        if (p.has_parent_path()) p = p.parent_path();  // build
        if (p.has_parent_path()) base = p.parent_path();  // lab5
    }
    if (base.empty()) base = fs::current_path() / "lab5";
    std::error_code ec;
    fs::create_directories(base / "db", ec);
    fs::create_directories(base / "logs", ec);
    fs::create_directories(base / "web", ec);
    cached_dir = base.string();
    return cached_dir;
}

std::string db_path() {
    return (fs::path(data_dir()) / "db" / "lab5.db").string();
}

std::string web_root() {
    return (fs::path(data_dir()) / "web").string();
}

}  // namespace lab5


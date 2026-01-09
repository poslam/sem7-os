#include "platform.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace lab3 {
namespace {

void ensure_parent_exists(const std::string& path) {
    fs::path p(path);
    if (p.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(p.parent_path(), ec);
    }
}

}  // namespace

std::int64_t pid() {
#ifdef _WIN32
    return static_cast<std::int64_t>(GetCurrentProcessId());
#else
    return static_cast<std::int64_t>(getpid());
#endif
}

bool is_process_alive(std::int64_t other_pid) {
    if (other_pid <= 0) return false;
#ifdef _WIN32
    HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(other_pid));
    if (!h) return false;
    DWORD code = WaitForSingleObject(h, 0);
    CloseHandle(h);
    return code == WAIT_TIMEOUT;
#else
    int r = kill(static_cast<pid_t>(other_pid), 0);
    return r == 0 || errno == EPERM;
#endif
}

std::string timestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03lld",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec,
                  static_cast<long long>(ms.count()));
    return buf;
}

std::string exe_path() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::string(buffer, buffer + len);
#else
    char buffer[4096];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len <= 0) return "";
    buffer[len] = '\0';
    return std::string(buffer);
#endif
}

std::string data_dir() {
    static std::string cached;
    if (!cached.empty()) return cached;
    fs::path base;
    const auto exe = exe_path();
    if (!exe.empty()) {
        fs::path p(exe);
        if (p.has_parent_path()) p = p.parent_path();       // bin
        if (p.has_parent_path()) p = p.parent_path();       // build
        if (p.has_parent_path()) base = p.parent_path();    // lab3
    }
    if (base.empty()) {
        base = fs::current_path() / "lab3";
    }
    std::error_code ec;
    fs::create_directories(base, ec);
    cached = base.string();
    return cached;
}

FileLock::FileLock(const std::string& path) {
    ensure_parent_exists(path);
#ifdef _WIN32
    handle_ = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle_ == INVALID_HANDLE_VALUE) {
        handle_ = nullptr;
        return;
    }
    OVERLAPPED ov{};
    if (!LockFileEx(handle_, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ov)) {
        CloseHandle(handle_);
        handle_ = nullptr;
    }
#else
    fd_ = ::open(path.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd_ < 0) return;
    struct flock fl{};
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // lock whole file
    if (fcntl(fd_, F_SETLKW, &fl) == -1) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
}

bool FileLock::locked() const {
#ifdef _WIN32
    return handle_ != nullptr;
#else
    return fd_ >= 0;
#endif
}

FileLock::~FileLock() {
#ifdef _WIN32
    if (handle_) {
        OVERLAPPED ov{};
        UnlockFileEx(handle_, 0, MAXDWORD, MAXDWORD, &ov);
        CloseHandle(handle_);
    }
#else
    if (fd_ >= 0) {
        struct flock fl{};
        fl.l_type = F_UNLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;
        fcntl(fd_, F_SETLKW, &fl);
        ::close(fd_);
    }
#endif
}

void log_line(const std::string& line) {
    const auto path = log_file_path();
    ensure_parent_exists(path);
    FileLock lock(path + ".lock");
    std::ofstream out(path, std::ios::app);
    out << line << "\n";
}

bool map_shared(SharedMap& m) {
    const auto path = shared_file_path();
    ensure_parent_exists(path);
#ifdef _WIN32
    HANDLE file = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    size.QuadPart = sizeof(SharedState);
    if (!SetFilePointerEx(file, size, nullptr, FILE_BEGIN) || !SetEndOfFile(file)) {
        CloseHandle(file);
        return false;
    }
    HANDLE mapping = CreateFileMappingA(file, nullptr, PAGE_READWRITE, 0, sizeof(SharedState), nullptr);
    if (!mapping) {
        CloseHandle(file);
        return false;
    }
    auto* ptr = static_cast<SharedState*>(MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedState)));
    if (!ptr) {
        CloseHandle(mapping);
        CloseHandle(file);
        return false;
    }
    m.file = file;
    m.mapping = mapping;
    m.ptr = ptr;
    return true;
#else
    int fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd < 0) return false;
    if (ftruncate(fd, sizeof(SharedState)) != 0) {
        ::close(fd);
        return false;
    }
    void* p = mmap(nullptr, sizeof(SharedState), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        ::close(fd);
        return false;
    }
    m.fd = fd;
    m.ptr = static_cast<SharedState*>(p);
    return true;
#endif
}

void unmap_shared(SharedMap& m) {
#ifdef _WIN32
    if (m.ptr) UnmapViewOfFile(m.ptr);
    if (m.mapping) CloseHandle(static_cast<HANDLE>(m.mapping));
    if (m.file) CloseHandle(static_cast<HANDLE>(m.file));
    m.mapping = nullptr;
    m.file = nullptr;
#else
    if (m.ptr) munmap(m.ptr, sizeof(SharedState));
    if (m.fd >= 0) close(m.fd);
    m.fd = -1;
#endif
    m.ptr = nullptr;
}

bool should_take_over(const SharedState& s, std::int64_t self) {
    const auto now = now_ms();
    const bool heartbeat_stale = (now - s.owner_heartbeat_ms) > 4000;
    const bool owner_dead = !is_process_alive(s.owner_pid);
    return s.owner_pid == 0 || owner_dead || heartbeat_stale;
}

std::int64_t launch_child(int mode) {
    std::string exe = exe_path();
    if (exe.empty()) return 0;
#ifdef _WIN32
    std::ostringstream cmd;
    cmd << '"' << exe << '"' << " --child=" << mode;
    std::string cmdline = cmd.str();
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::vector<char> buf(cmdline.begin(), cmdline.end());
    buf.push_back('\0');
    BOOL ok = CreateProcessA(nullptr, buf.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
    std::int64_t child_pid = 0;
    if (ok) {
        child_pid = static_cast<std::int64_t>(pi.dwProcessId);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    return child_pid;
#else
    pid_t cpid = fork();
    if (cpid == 0) {
        execl(exe.c_str(), exe.c_str(), (mode == 1 ? "--child=1" : "--child=2"), (char*)nullptr);
        _exit(127);
    }
    return cpid > 0 ? static_cast<std::int64_t>(cpid) : 0;
#endif
}

std::string shared_file_path() {
    fs::path p = fs::path(data_dir()) / "lab3_shared.bin";
    return p.string();
}

std::string log_file_path() {
    fs::path p = fs::path(data_dir()) / "logs" / "lab3.log";
    return p.string();
}

}  // namespace lab3


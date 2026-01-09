#include "bg_process.h"

#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

void set_err(std::string* out, const std::string& msg) {
    if (out) {
        *out = msg;
    }
}

#ifdef _WIN32
std::string quote_arg(const std::string& arg) {
    if (arg.empty()) {
        return "\"\"";
    }

    bool need_quotes = arg.find_first_of(" \t\"") != std::string::npos;
    if (!need_quotes) {
        return arg;
    }

    std::string result;
    result.reserve(arg.size() + 2);
    result.push_back('"');

    unsigned backslashes = 0;
    for (char ch : arg) {
        if (ch == '\\') {
            ++backslashes;
            continue;
        }

        if (ch == '"') {
            result.append(backslashes * 2 + 1, '\\');
            result.push_back('"');
            backslashes = 0;
            continue;
        }

        result.append(backslashes, '\\');
        backslashes = 0;
        result.push_back(ch);
    }

    result.append(backslashes, '\\');
    result.push_back('"');
    return result;
}

std::string make_command_line(const std::vector<std::string>& args) {
    std::ostringstream oss;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) {
            oss << ' ';
        }
        oss << quote_arg(args[i]);
    }
    return oss.str();
}

std::string format_last_error() {
    DWORD code = GetLastError();
    if (code == 0) return {};

    LPSTR buffer = nullptr;
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer),
        0,
        nullptr);

    std::string message = "Win32 error " + std::to_string(code);
    if (size && buffer) {
        message += ": ";
        message.append(buffer, size);
    }

    if (buffer) {
        LocalFree(buffer);
    }
    return message;
}
#else
std::vector<char*> to_argv(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const auto& a : args) {
        argv.push_back(const_cast<char*>(a.c_str()));
    }
    argv.push_back(nullptr);
    return argv;
}
#endif

}  // namespace

namespace bgproc {

bool start(const std::vector<std::string>& args, ProcessHandle& handle, std::string* err) {
    if (args.empty()) {
        set_err(err, "No executable specified");
        return false;
    }

    close(handle);

#ifdef _WIN32
    std::string cmdline = make_command_line(args);
    std::vector<char> buffer(cmdline.begin(), cmdline.end());
    buffer.push_back('\0');

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessA(
        nullptr,
        buffer.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi);

    if (!ok) {
        set_err(err, format_last_error());
        return false;
    }

    // Поток нам не нужен для ожидания.
    CloseHandle(pi.hThread);
    handle.process = pi.hProcess;
    return true;
#else
    pid_t pid = fork();
    if (pid == 0) {
        auto argv = to_argv(args);
        execvp(argv[0], argv.data());
        _exit(127);  // execvp failed
    }

    if (pid < 0) {
        set_err(err, "fork failed: " + std::to_string(errno));
        return false;
    }

    handle.pid = static_cast<int>(pid);
    return true;
#endif
}

bool wait(ProcessHandle& handle, int& exit_code, int timeout_ms, std::string* err) {
#ifdef _WIN32
    if (!handle.process) {
        set_err(err, "Process handle is empty");
        return false;
    }

    DWORD wait_time = timeout_ms < 0 ? INFINITE : static_cast<DWORD>(timeout_ms);
    DWORD res = WaitForSingleObject(handle.process, wait_time);

    if (res == WAIT_TIMEOUT) {
        set_err(err, "Timed out");
        return false;
    }

    if (res != WAIT_OBJECT_0) {
        set_err(err, format_last_error());
        return false;
    }

    DWORD code = 0;
    if (!GetExitCodeProcess(handle.process, &code)) {
        set_err(err, format_last_error());
        return false;
    }

    CloseHandle(handle.process);
    handle.process = nullptr;
    exit_code = static_cast<int>(code);
    return true;
#else
    if (handle.pid <= 0) {
        set_err(err, "PID is empty");
        return false;
    }

    int status = 0;
    if (timeout_ms < 0) {
        while (true) {
            pid_t r = waitpid(handle.pid, &status, 0);
            if (r == -1 && errno == EINTR) {
                continue;
            } else if (r == handle.pid) {
                break;
            } else {
                set_err(err, "waitpid failed: " + std::to_string(errno));
                return false;
            }
        }
    } else {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
        while (true) {
            pid_t r = waitpid(handle.pid, &status, WNOHANG);
            if (r == 0) {
                if (std::chrono::steady_clock::now() >= deadline) {
                    set_err(err, "Timed out");
                    return false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
                continue;
            } else if (r == -1) {
                if (errno == EINTR) {
                    continue;
                }
                set_err(err, "waitpid failed: " + std::to_string(errno));
                return false;
            } else {
                break;
            }
        }
    }

    handle.pid = -1;
    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        exit_code = 128 + WTERMSIG(status);
    } else {
        exit_code = status;
    }
    return true;
#endif
}

void close(ProcessHandle& handle) {
#ifdef _WIN32
    if (handle.process) {
        CloseHandle(handle.process);
        handle.process = nullptr;
    }
#else
    handle.pid = -1;
#endif
}

}  // namespace bgproc

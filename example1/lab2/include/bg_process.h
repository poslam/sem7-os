#pragma once

#include <string>
#include <vector>

namespace bgproc {

struct ProcessHandle {
#ifdef _WIN32
    void* process = nullptr;  // HANDLE
#else
    int pid = -1;
#endif
};

// Запускает процесс в фоне. args[0] — исполняемый файл, остальные — аргументы.
// Возвращает true при успешном старте.
bool start(const std::vector<std::string>& args, ProcessHandle& handle, std::string* err = nullptr);

// Ожидает завершения процесса. timeout_ms < 0 — ждать бесконечно.
// Возвращает true при успешном ожидании, exit_code заполняется кодом возврата.
bool wait(ProcessHandle& handle, int& exit_code, int timeout_ms = -1, std::string* err = nullptr);

// Освобождает ресурсы дескриптора (если осталось что-то открыто).
void close(ProcessHandle& handle);

}  // namespace bgproc

#include "app.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "platform.h"
#include "shared.h"

namespace lab3 {

namespace {

void run_child(int mode, SharedMap& map) {
    const auto self = pid();
    log_line(timestamp() + " child" + std::to_string(mode) + " start pid=" + std::to_string(self));
    {
        FileLock lock(shared_file_path() + ".lock");
        if (lock.locked() && map.ptr) {
            if (mode == 1) {
                map.ptr->counter += 10;
            } else {
                map.ptr->counter *= 2;
            }
        }
    }
    if (mode == 2) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        FileLock lock(shared_file_path() + ".lock");
        if (lock.locked() && map.ptr) {
            map.ptr->counter /= 2;
        }
    }
    log_line(timestamp() + " child" + std::to_string(mode) + " exit pid=" + std::to_string(self));
}

}  // namespace

void run_app(bool is_child, int child_mode) {
    SharedMap map;
    if (!map_shared(map) || !map.ptr) {
        std::cerr << "Failed to map shared state\n";
        return;
    }

    const auto self = pid();
    log_line(timestamp() + " start pid=" + std::to_string(self));

    if (is_child) {
        run_child(child_mode, map);
        unmap_shared(map);
        return;
    }

    std::atomic<bool> running{true};

    std::thread input_thread([&]() {
        std::string line;
        while (running && std::getline(std::cin, line)) {
            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;
            if (cmd == "set") {
                long long val;
                if (iss >> val) {
                    FileLock lock(shared_file_path() + ".lock");
                    if (lock.locked() && map.ptr) {
                        map.ptr->counter = val;
                        log_line(timestamp() + " pid=" + std::to_string(self) + " set counter=" + std::to_string(val));
                    }
                } else {
                    std::cout << "Usage: set <number>\n";
                }
            } else if (cmd == "exit" || cmd == "quit") {
                running = false;
                break;
            }
        }
        running = false;
    });

    std::thread counter_thread([&]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            FileLock lock(shared_file_path() + ".lock");
            if (lock.locked() && map.ptr) {
                map.ptr->counter += 1;
            }
        }
    });

    auto last_log = now_ms();
    auto last_spawn = now_ms();

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const auto t = now_ms();

        // Owner election and heartbeat
        {
            FileLock lock(shared_file_path() + ".lock");
            if (lock.locked() && map.ptr) {
                if (map.ptr->owner_pid == self) {
                    map.ptr->owner_heartbeat_ms = t;
                } else if (should_take_over(*map.ptr, self)) {
                    map.ptr->owner_pid = self;
                    map.ptr->owner_heartbeat_ms = t;
                    log_line(timestamp() + " pid=" + std::to_string(self) + " became owner");
                }
            }
        }

        bool am_owner = false;
        {
            FileLock lock(shared_file_path() + ".lock");
            if (lock.locked() && map.ptr) {
                am_owner = (map.ptr->owner_pid == self);
            }
        }

        if (am_owner) {
            if (t - last_log >= 1000) {
                last_log = t;
                std::int64_t cnt = 0;
                {
                    FileLock lock(shared_file_path() + ".lock");
                    if (lock.locked() && map.ptr) {
                        cnt = map.ptr->counter;
                    }
                }
                log_line(timestamp() + " pid=" + std::to_string(self) + " counter=" + std::to_string(cnt));
            }

            if (t - last_spawn >= 3000) {
                last_spawn = t;
                bool child_running = false;
                {
                    FileLock lock(shared_file_path() + ".lock");
                    if (lock.locked() && map.ptr) {
                        if (map.ptr->child1_pid && is_process_alive(map.ptr->child1_pid)) child_running = true;
                        else map.ptr->child1_pid = 0;
                        if (map.ptr->child2_pid && is_process_alive(map.ptr->child2_pid)) child_running = true;
                        else map.ptr->child2_pid = 0;
                    }
                }

                if (child_running) {
                    log_line(timestamp() + " pid=" + std::to_string(self) + " skip spawn: child still running");
                } else {
                    auto p1 = launch_child(1);
                    auto p2 = launch_child(2);
                    {
                        FileLock lock(shared_file_path() + ".lock");
                        if (lock.locked() && map.ptr) {
                            map.ptr->child1_pid = p1;
                            map.ptr->child2_pid = p2;
                        }
                    }
                    if (!p1 || !p2) {
                        log_line(timestamp() + " pid=" + std::to_string(self) + " failed to spawn child(s)");
                    } else {
                        log_line(timestamp() + " pid=" + std::to_string(self) + " spawned children " + std::to_string(p1) + ", " + std::to_string(p2));
                    }
                }
            }
        }
    }

    running = false;
    if (input_thread.joinable()) input_thread.join();
    if (counter_thread.joinable()) counter_thread.join();

    log_line(timestamp() + " exit pid=" + std::to_string(self));
    unmap_shared(map);
}

}  // namespace lab3

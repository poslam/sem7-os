#include <atomic>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "common.h"
#include "logging.h"
#include "sample.h"
#include "simulator.h"

using namespace std::chrono;

namespace lab4 {
namespace {
std::atomic<bool> g_running{true};

void signal_handler(int) { g_running = false; }
}

int run_main(bool simulate) {
    std::signal(SIGINT, signal_handler);
#ifndef _WIN32
    std::signal(SIGTERM, signal_handler);
#endif

    std::cout << "lab4 temperature logger. Mode: " << (simulate ? "simulate" : "stdin") << "\n";

    Simulator sim;
    if (simulate) sim.start();

    Accum hour_acc;
    Accum day_acc;

    auto now = Clock::now();
    auto last_hour = hour_of(now);
    auto last_day = day_of_year(now);
    auto last_year = year_of(now);

    int sample_count = 0;

    auto flush_hour = [&](const TimePoint& ts) {
        if (hour_acc.count == 0) return;
        Sample avg{ts, hour_acc.avg()};
        append_line(hourly_log_path(), format_line(avg));
        prune_log(hourly_log_path(), now_ms() - static_cast<std::int64_t>(30LL * 24 * 60 * 60 * 1000));
        hour_acc.reset();
    };

    auto flush_day = [&](const TimePoint& ts) {
        if (day_acc.count == 0) return;
        Sample avg{ts, day_acc.avg()};
        append_line(daily_log_path(), format_line(avg));
        day_acc.reset();
    };

    auto process_sample = [&](const Sample& s) {
        const auto h = hour_of(s.ts);
        const auto d = day_of_year(s.ts);
        const auto y = year_of(s.ts);

        if (h != last_hour) {
            flush_hour(s.ts);
        }
        last_hour = h;

        if (d != last_day) {
            flush_day(s.ts);
        }
        last_day = d;
        last_year = y;

        hour_acc.add(s.value);
        day_acc.add(s.value);

        append_line(measurements_log_path(), format_line(s));
        ++sample_count;
        if (sample_count % 50 == 0) {
            prune_log(measurements_log_path(), now_ms() - static_cast<std::int64_t>(24LL * 60 * 60 * 1000));
            prune_log(hourly_log_path(), now_ms() - static_cast<std::int64_t>(30LL * 24 * 60 * 60 * 1000));
            // daily log: keep only current year
            std::ifstream in(daily_log_path());
            if (in.is_open()) {
                std::vector<std::string> keep;
                std::string line;
                while (std::getline(in, line)) {
                    auto ms = parse_epoch_ms(line);
                    if (!ms) continue;
                    TimePoint tp = TimePoint(milliseconds(*ms));
                    if (year_of(tp) == year_of(Clock::now())) keep.push_back(line);
                }
                in.close();
                std::ofstream out(daily_log_path(), std::ios::trunc);
                for (const auto& l : keep) out << l << '\n';
            }
        }
    };

    while (g_running) {
        Sample s;
        bool got = false;
        if (simulate) {
            got = sim.pop(s);
        } else {
            std::string line;
            if (!std::getline(std::cin, line)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            std::istringstream iss(line);
            double val;
            if (!(iss >> val)) {
                std::cerr << "Invalid input, expected number.\n";
                continue;
            }
            s.ts = Clock::now();
            s.value = val;
            got = true;
        }

        if (got) {
            process_sample(s);
        } else if (!simulate) {
            break;
        }
    }

    if (simulate) sim.stop();

    flush_hour(Clock::now());
    flush_day(Clock::now());

    return 0;
}

}  // namespace lab4

int main(int argc, char* argv[]) {
    bool simulate = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--simulate") simulate = true;
    }
    return lab4::run_main(simulate);
}

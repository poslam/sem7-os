#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "common.h"
#include "db.h"
#include "logging.h"
#include "sample.h"
#include "simulator.h"
#include "http_server.h"

using namespace std::chrono;
namespace fs = std::filesystem;

namespace lab5 {
namespace {
std::atomic<bool> g_running{true};

void signal_handler(int) { g_running = false; }

std::string samples_to_json(const std::vector<Sample>& v) {
    std::ostringstream oss;
    oss << '[';
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) oss << ',';
        auto ms = duration_cast<milliseconds>(v[i].ts.time_since_epoch()).count();
        oss << "[" << ms << "," << v[i].value << "]";
    }
    oss << ']';
    return oss.str();
}

}  // namespace

void request_stop() { g_running = false; }

int run_main(bool simulate) {
    g_running = true;
    std::signal(SIGINT, signal_handler);
#ifndef _WIN32
    std::signal(SIGTERM, signal_handler);
#endif

    std::cout << "lab5 temperature logger. Mode: " << (simulate ? "simulate" : "stdin") << "\n";

    Database db;
    std::string err;
    if (!db.open(db_path(), err)) {
        std::cerr << "DB open failed: " << err << "\n";
        return 1;
    }

    Simulator sim;
    if (simulate) sim.start();

    Accum hour_acc;
    Accum day_acc;

    auto now = Clock::now();
    auto last_hour = hour_of(now);
    auto last_day = day_of_year(now);

    int sample_count = 0;

    auto flush_hour = [&](const TimePoint& ts) {
        if (hour_acc.count == 0) return;
        Sample avg{ts, hour_acc.avg()};
        db.insert_hourly(avg, err);
        db.prune_hourly(now_ms() - static_cast<std::int64_t>(30LL * 24 * 60 * 60 * 1000), err);
        hour_acc.reset();
    };

    auto flush_day = [&](const TimePoint& ts) {
        if (day_acc.count == 0) return;
        Sample avg{ts, day_acc.avg()};
        db.insert_daily(avg, err);
        db.prune_daily_current_year(err);
        day_acc.reset();
    };

    const fs::path web_root_path = fs::path(web_root());
    const fs::path dist_root = web_root_path / "dist";
    const fs::path static_root = fs::exists(dist_root) ? dist_root : web_root_path;

    auto serve_static = [&](const std::string& req_path) -> std::optional<std::pair<std::string, std::string>> {
        std::string rel = req_path;
        if (!rel.empty() && rel[0] == '/') rel.erase(0, 1);
        fs::path target = static_root / (rel.empty() ? fs::path("index.html") : fs::path(rel));
        std::error_code ec;
        const bool is_dir = fs::is_directory(target, ec);
        if (ec) return std::nullopt;
        if (is_dir) target /= "index.html";
        if (!fs::exists(target, ec) || !fs::is_regular_file(target, ec)) return std::nullopt;
        std::ifstream in(target, std::ios::binary);
        if (!in.is_open()) return std::nullopt;
        std::ostringstream buf;
        buf << in.rdbuf();
        auto ext = target.extension().string();
        std::string content_type = "text/plain";
        if (ext == ".html") content_type = "text/html; charset=utf-8";
        else if (ext == ".js") content_type = "application/javascript";
        else if (ext == ".css") content_type = "text/css";
        else if (ext == ".json") content_type = "application/json";
        else if (ext == ".svg") content_type = "image/svg+xml";
        else if (ext == ".ico") content_type = "image/x-icon";
        else if (ext == ".png") content_type = "image/png";
        else if (ext == ".jpg" || ext == ".jpeg") content_type = "image/jpeg";
        else if (ext == ".woff2") content_type = "font/woff2";
        return std::make_pair(buf.str(), content_type);
    };

    HttpServer server;
    auto handler = [&](const std::string& req) -> std::pair<std::string, std::string> {
        auto pos = req.find(' ');
        if (pos == std::string::npos) return {"{}", "application/json"};
        auto pos2 = req.find(' ', pos + 1);
        auto path = req.substr(pos + 1, pos2 - pos - 1);
        auto path_no_query = path;
        auto qmark = path_no_query.find('?');
        if (qmark != std::string::npos) path_no_query = path_no_query.substr(0, qmark);

        if (path == "/api/current") {
            auto latest = db.latest_measurement(err);
            if (!latest) return {"{}", "application/json"};
            std::ostringstream o;
            auto ms = duration_cast<milliseconds>(latest->ts.time_since_epoch()).count();
            o << "{\"epoch_ms\":" << ms << ",\"value\":" << latest->value << "}";
            return {o.str(), "application/json"};
        }

        if (path.rfind("/api/stats", 0) == 0) {
            std::string table = "measurements";
            std::int64_t start = now_ms() - 3600 * 1000;
            std::int64_t end = now_ms();
            auto qpos = path.find('?');
            if (qpos != std::string::npos) {
                auto qs = path.substr(qpos + 1);
                std::istringstream iss(qs);
                std::string kv;
                while (std::getline(iss, kv, '&')) {
                    auto eq = kv.find('=');
                    if (eq == std::string::npos) continue;
                    auto key = kv.substr(0, eq);
                    auto val = kv.substr(eq + 1);
                    if (key == "bucket") table = val == "hourly" ? "hourly_avg" : (val == "daily" ? "daily_avg" : "measurements");
                    else if (key == "start") start = std::stoll(val);
                    else if (key == "end") end = std::stoll(val);
                }
            }
            std::vector<Sample> out;
            if (!db.query_range(table, start, end, out, err)) return {"{}", "application/json"};
            std::ostringstream o;
            o << "{\"bucket\":\"" << table << "\",\"data\":" << samples_to_json(out) << "}";
            return {o.str(), "application/json"};
        }

        if (path_no_query == "/" || path_no_query == "/index.html") {
            if (auto res = serve_static("index.html")) return *res;
        }

        if (auto res = serve_static(path_no_query)) return *res;

        return {"{}", "application/json"};
    };

    if (!server.start(8080, handler, err)) {
        std::cerr << "HTTP start failed: " << err << "\n";
    }

    auto process_sample = [&](const Sample& s) {
        const auto h = hour_of(s.ts);
        const auto d = day_of_year(s.ts);

        if (h != last_hour) {
            flush_hour(s.ts);
        }
        last_hour = h;

        if (d != last_day) {
            flush_day(s.ts);
        }
        last_day = d;

        hour_acc.add(s.value);
        day_acc.add(s.value);

        db.insert_measurement(s, err);
        ++sample_count;
        if (sample_count % 50 == 0) {
            db.prune_measurements(now_ms() - static_cast<std::int64_t>(24LL * 60 * 60 * 1000), err);
            db.prune_hourly(now_ms() - static_cast<std::int64_t>(30LL * 24 * 60 * 60 * 1000), err);
            db.prune_daily_current_year(err);
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
    server.stop();

    flush_hour(Clock::now());
    flush_day(Clock::now());

    return 0;
}

}  // namespace lab5

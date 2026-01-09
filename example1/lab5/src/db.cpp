#include "db.h"

#include <sqlite3.h>
#include <chrono>
#include <sstream>

namespace lab5 {

Database::Database() = default;
Database::~Database() {
    if (db_) sqlite3_close(db_);
}

bool Database::exec(const std::string& sql, std::string& err) {
    char* errmsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
        if (errmsg) {
            err = errmsg;
            sqlite3_free(errmsg);
        }
        return false;
    }
    return true;
}

bool Database::open(const std::string& path, std::string& err) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        err = sqlite3_errmsg(db_);
        return false;
    }
    const char* ddl = R"(
        PRAGMA journal_mode=WAL;
        CREATE TABLE IF NOT EXISTS measurements(epoch_ms INTEGER PRIMARY KEY, iso TEXT, value REAL);
        CREATE TABLE IF NOT EXISTS hourly_avg(epoch_ms INTEGER PRIMARY KEY, iso TEXT, value REAL);
        CREATE TABLE IF NOT EXISTS daily_avg(epoch_ms INTEGER PRIMARY KEY, iso TEXT, value REAL);
    )";
    return exec(ddl, err);
}

bool Database::insert_measurement(const Sample& s, std::string& err) {
    std::ostringstream oss;
    oss << "INSERT INTO measurements(epoch_ms, iso, value) VALUES("
        << std::chrono::duration_cast<std::chrono::milliseconds>(s.ts.time_since_epoch()).count()
        << ",'" << iso_time(s.ts) << "'," << s.value << ");";
    return exec(oss.str(), err);
}

bool Database::insert_hourly(const Sample& s, std::string& err) {
    std::ostringstream oss;
    oss << "INSERT INTO hourly_avg(epoch_ms, iso, value) VALUES("
        << std::chrono::duration_cast<std::chrono::milliseconds>(s.ts.time_since_epoch()).count()
        << ",'" << iso_time(s.ts) << "'," << s.value << ");";
    return exec(oss.str(), err);
}

bool Database::insert_daily(const Sample& s, std::string& err) {
    std::ostringstream oss;
    oss << "INSERT INTO daily_avg(epoch_ms, iso, value) VALUES("
        << std::chrono::duration_cast<std::chrono::milliseconds>(s.ts.time_since_epoch()).count()
        << ",'" << iso_time(s.ts) << "'," << s.value << ");";
    return exec(oss.str(), err);
}

std::optional<Sample> Database::latest_measurement(std::string& err) {
    const char* sql = "SELECT epoch_ms, value FROM measurements ORDER BY epoch_ms DESC LIMIT 1";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        err = sqlite3_errmsg(db_);
        return std::nullopt;
    }
    std::optional<Sample> res;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::int64_t ms = sqlite3_column_int64(stmt, 0);
        double v = sqlite3_column_double(stmt, 1);
        res = Sample{TimePoint(std::chrono::milliseconds(ms)), v};
    }
    sqlite3_finalize(stmt);
    return res;
}

bool Database::query_range(const std::string& table, std::int64_t start_ms, std::int64_t end_ms, std::vector<Sample>& out, std::string& err) {
    std::ostringstream oss;
    oss << "SELECT epoch_ms, value FROM " << table << " WHERE epoch_ms BETWEEN " << start_ms << " AND " << end_ms << " ORDER BY epoch_ms";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, oss.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        err = sqlite3_errmsg(db_);
        return false;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::int64_t ms = sqlite3_column_int64(stmt, 0);
        double v = sqlite3_column_double(stmt, 1);
        out.push_back(Sample{TimePoint(std::chrono::milliseconds(ms)), v});
    }
    sqlite3_finalize(stmt);
    return true;
}

bool Database::prune_measurements(std::int64_t cutoff_ms, std::string& err) {
    std::ostringstream oss;
    oss << "DELETE FROM measurements WHERE epoch_ms < " << cutoff_ms;
    return exec(oss.str(), err);
}

bool Database::prune_hourly(std::int64_t cutoff_ms, std::string& err) {
    std::ostringstream oss;
    oss << "DELETE FROM hourly_avg WHERE epoch_ms < " << cutoff_ms;
    return exec(oss.str(), err);
}

bool Database::prune_daily_current_year(std::string& err) {
    const auto now = Clock::now();
    auto tt = Clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    int year = tm.tm_year + 1900;
    std::ostringstream oss;
    oss << "DELETE FROM daily_avg WHERE strftime('%Y', datetime(epoch_ms/1000, 'unixepoch')) != '" << year << "'";
    return exec(oss.str(), err);
}

}  // namespace lab5

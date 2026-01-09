#include "logging.h"
#include "common.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace lab5 {

std::string format_line(const Sample& s) {
    auto ms_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(s.ts.time_since_epoch()).count();
    std::ostringstream oss;
    oss << ms_epoch << ';' << iso_time(s.ts) << ';' << std::fixed << std::setprecision(2) << s.value;
    return oss.str();
}

std::optional<std::int64_t> parse_epoch_ms(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    if (!std::getline(iss, token, ';')) return std::nullopt;
    try {
        return std::stoll(token);
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace lab5

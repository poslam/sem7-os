#pragma once

#include <optional>
#include <string>

#include "sample.h"

namespace lab5 {

std::string format_line(const Sample& s);
std::optional<std::int64_t> parse_epoch_ms(const std::string& line);

}  // namespace lab5

#include "names.hpp"

#include <algorithm>

namespace cpm::args::detail {

namespace {

bool is_lower(char c) noexcept { return c >= 'a' && c <= 'z'; }
bool is_digit(char c) noexcept { return c >= '0' && c <= '9'; }

}  // namespace

bool is_long_name(std::string_view name) noexcept {
    return !name.empty() && is_lower(name.front()) &&
           std::all_of(name.begin(), name.end(), [](char c) { return is_lower(c) || is_digit(c) || c == '-'; });
}

bool is_short_name(char name) noexcept { return is_lower(name) || is_digit(name) || (name >= 'A' && name <= 'Z'); }

bool is_command_name(std::string_view name) noexcept { return is_long_name(name); }

bool is_display_name(std::string_view name) noexcept {
    return !name.empty() && !name.starts_with('-') &&
           std::none_of(name.begin(), name.end(), [](char c) { return c == ' ' || c == '\t' || c == '\n'; });
}

}  // namespace cpm::args::detail

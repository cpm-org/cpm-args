// What names may look like (docs/spec.md §3.1).
#pragma once

#include <string_view>

namespace cpm::args::detail {

// The part after "--": lowercase letters, digits and '-', starting with a letter.
bool is_long_name(std::string_view name) noexcept;

// The letter or digit after "-".
bool is_short_name(char name) noexcept;

// A command's name: as a long name.
bool is_command_name(std::string_view name) noexcept;

// A positional argument's display name: not empty, no whitespace, and not starting with '-'.
bool is_display_name(std::string_view name) noexcept;

}  // namespace cpm::args::detail

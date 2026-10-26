// Usage and help text (docs/spec.md §8).
#pragma once

#include <cpm/args/command.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace cpm::args::detail {

// "Usage: cpm build [OPTIONS] FILE...", without a line break.
std::string usage_line(const Command& command);

// The whole help of `command`, ending in a line break. `has_version`: the root has -V/--version.
std::string help_text(const Command& command, std::size_t width, bool has_version);

// `text` split into lines of at most `width` columns at spaces; a word longer than that gets a line of its own.
std::vector<std::string> wrap(std::string_view text, std::size_t width);

}  // namespace cpm::args::detail

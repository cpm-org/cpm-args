// Suggestions for names that were probably mistyped (docs/spec.md §7).
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cpm::args::detail {

// The optimal string alignment distance: insertions, deletions, substitutions, and swaps of adjacent characters.
std::size_t edit_distance(std::string_view a, std::string_view b);

// The candidate closest to `word`, if it is within a distance of 2 and no other candidate is as close.
std::optional<std::string> closest(std::string_view word, const std::vector<std::string>& candidates);

}  // namespace cpm::args::detail

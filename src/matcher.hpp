// Walking a command line against the commands (docs/spec.md §5, §6, §7).
#pragma once

#include <cpm/args/command.hpp>
#include <cpm/args/environment.hpp>
#include <cpm/args/parser.hpp>

#include <span>
#include <string_view>

namespace cpm::args::detail {

// Fills in the variables of the arguments given, then those with environment variables, and checks what is required.
// Help and the version, when asked for, win over everything else. Throws UsageError.
Outcome match(const Command& root, std::span<const std::string_view> args, const Environment& environment,
              bool has_version);

}  // namespace cpm::args::detail

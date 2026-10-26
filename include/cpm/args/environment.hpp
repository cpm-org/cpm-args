// Environment variables, read through a function so tests can supply their own (docs/spec.md §6).
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace cpm::args {

// The value of an environment variable, or nullopt when it isn't set.
using Environment = std::function<std::optional<std::string>(std::string_view name)>;

// The process's environment, through std::getenv.
Environment system_environment();

}  // namespace cpm::args

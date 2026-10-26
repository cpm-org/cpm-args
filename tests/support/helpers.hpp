// Shared by the tests: parsing a list of arguments with a chosen environment, and catching usage errors.
#pragma once

#include <cpm/args.hpp>

#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace support {

namespace args = cpm::args;

// An environment with these variables and no others.
inline args::Environment environment(std::map<std::string, std::string, std::less<>> variables = {}) {
    return [variables](std::string_view name) -> std::optional<std::string> {
        if (auto it = variables.find(name); it != variables.end()) return it->second;
        return std::nullopt;
    };
}

inline args::Outcome parse(args::Parser& parser, std::initializer_list<std::string_view> arguments,
                           const args::Environment& env = environment()) {
    const std::vector<std::string_view> list(arguments);
    return parser.parse(list, env);
}

// The message of the UsageError that parsing throws, or "" when it parses.
inline std::string usage_error(args::Parser& parser, std::initializer_list<std::string_view> arguments,
                               const args::Environment& env = environment()) {
    try {
        parse(parser, arguments, env);
    } catch (const args::UsageError& error) {
        return error.what();
    }
    return "";
}

}  // namespace support

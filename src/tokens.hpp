// What each command-line argument looks like (docs/spec.md §5).
#pragma once

#include <optional>
#include <string_view>

namespace cpm::args::detail {

struct Token {
    enum class Kind {
        long_option,  // --name or --name=value
        short_group,  // -abc: one or more short names, perhaps with a value attached
        separator,    // --
        plain,        // anything else, "-" included
    };
    Kind kind = Kind::plain;
    std::string_view name;                   // long_option: after "--", up to any '='; short_group: after "-"
    std::optional<std::string_view> value;   // long_option: after '='
};

Token classify(std::string_view argument) noexcept;

// Whether the text is a number such as -5 or -0.5, which a command line means as a value rather than as options.
bool is_number(std::string_view text) noexcept;

}  // namespace cpm::args::detail

#include "tokens.hpp"

#include <cpm/args/reader.hpp>

namespace cpm::args::detail {

Token classify(std::string_view argument) noexcept {
    if (argument == "--") return {Token::Kind::separator, {}, std::nullopt};
    if (argument.starts_with("--")) {
        const std::string_view rest = argument.substr(2);
        const std::size_t equals = rest.find('=');
        if (equals == std::string_view::npos) return {Token::Kind::long_option, rest, std::nullopt};
        return {Token::Kind::long_option, rest.substr(0, equals), rest.substr(equals + 1)};
    }
    if (argument.size() > 1 && argument.front() == '-') return {Token::Kind::short_group, argument.substr(1), std::nullopt};
    return {Token::Kind::plain, argument, std::nullopt};
}

bool is_number(std::string_view text) noexcept {
    double value = 0;
    return read_double(text, value) != Number::not_a_number;
}

}  // namespace cpm::args::detail

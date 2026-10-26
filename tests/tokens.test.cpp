// Names, tokens and suggestions (src/names, src/tokens, src/suggest).

#include <cpm/test.hpp>

#include <string>
#include <vector>

#include "names.hpp"
#include "suggest.hpp"
#include "tokens.hpp"

namespace detail = cpm::args::detail;
using Kind = detail::Token::Kind;

TEST_CASE("names") {
    CHECK(detail::is_long_name("port"));
    CHECK(detail::is_long_name("dry-run"));
    CHECK(detail::is_long_name("x2"));
    CHECK(!detail::is_long_name(""));
    CHECK(!detail::is_long_name("Port"));
    CHECK(!detail::is_long_name("2x"));
    CHECK(!detail::is_long_name("-x"));
    CHECK(!detail::is_long_name("a_b"));
    CHECK(detail::is_short_name('p'));
    CHECK(detail::is_short_name('P'));
    CHECK(detail::is_short_name('2'));
    CHECK(!detail::is_short_name('-'));
    CHECK(detail::is_display_name("FILE"));
    CHECK(!detail::is_display_name("-FILE"));
    CHECK(!detail::is_display_name("A FILE"));
}

TEST_CASE("arguments are classified by their shape") {
    const detail::Token long_option = detail::classify("--port");
    CHECK(long_option.kind == Kind::long_option);
    CHECK(long_option.name == "port");
    CHECK(!long_option.value.has_value());

    const detail::Token with_value = detail::classify("--port=80=x");
    CHECK(with_value.name == "port");
    CHECK(with_value.value == std::optional<std::string_view>("80=x"));

    const detail::Token empty_value = detail::classify("--name=");
    CHECK(empty_value.value == std::optional<std::string_view>(""));

    const detail::Token group = detail::classify("-vp8080");
    CHECK(group.kind == Kind::short_group);
    CHECK(group.name == "vp8080");

    CHECK(detail::classify("--").kind == Kind::separator);
    CHECK(detail::classify("-").kind == Kind::plain);
    CHECK(detail::classify("file.txt").kind == Kind::plain);
    CHECK(detail::classify("").kind == Kind::plain);
}

TEST_CASE("numbers are told apart from options") {
    CHECK(detail::is_number("-5"));
    CHECK(detail::is_number("-0.5"));
    CHECK(detail::is_number("-1e3"));
    CHECK(!detail::is_number("-v"));
    CHECK(!detail::is_number("-5x"));
}

TEST_CASE("edit distances count swaps as one") {
    CHECK(detail::edit_distance("port", "port") == 0);
    CHECK(detail::edit_distance("prot", "port") == 1);   // a swap
    CHECK(detail::edit_distance("pot", "port") == 1);    // an insertion
    CHECK(detail::edit_distance("porst", "port") == 1);  // a deletion
    CHECK(detail::edit_distance("bort", "port") == 1);   // a substitution
    CHECK(detail::edit_distance("", "abc") == 3);
}

TEST_CASE("the closest name within 2, unless tied") {
    const std::vector<std::string> names{"--port", "--verbose", "--version"};
    CHECK(detail::closest("--prot", names) == std::optional<std::string>("--port"));
    CHECK(detail::closest("--verbos", names) == std::optional<std::string>("--verbose"));
    CHECK(!detail::closest("--colour", names).has_value());       // too far
    CHECK(!detail::closest("--versio", {"--version", "--versiox"}).has_value());  // a tie
}

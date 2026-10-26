// The command line (docs/spec.md §3.2, §5): every syntax form and every kind of argument.

#include <cpm/args.hpp>
#include <cpm/test.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "support/helpers.hpp"

namespace args = cpm::args;
using support::parse;
using Strings = std::vector<std::string>;

namespace {

enum class Mode { fast, small };

// A small program: serve [--port N] [--verbose] [--header H]... [ROOT]
struct Serve {
    args::Parser parser{"serve", "Serves a directory over HTTP"};
    int port = 8080;
    bool verbose = false;
    int level = 0;
    Strings headers;
    std::optional<int> limit;
    std::string root = ".";

    Serve() {
        parser.option(port, "--port", "-p");
        parser.flag(verbose, "--verbose", "-v");
        parser.counter(level, "--level", "-l");
        parser.option(headers, "--header", "-H");
        parser.option(limit, "--limit");
        parser.positional(root, "ROOT").optional();
    }
};

}  // namespace

TEST_CASE("starting values are the defaults") {
    Serve s;
    const args::Outcome outcome = parse(s.parser, {});
    CHECK(outcome.action == args::Outcome::Action::run);
    CHECK(outcome.command == &s.parser);
    CHECK(outcome.path.empty());
    CHECK(s.port == 8080);
    CHECK(!s.verbose);
    CHECK(s.headers.empty());
    CHECK(!s.limit.has_value());
    CHECK(s.root == ".");
}

TEST_CASE("long options take values after a space or an =") {
    Serve s;
    parse(s.parser, {"--port", "80", "--header=A: 1", "--header", "B: 2", "--limit=5"});
    CHECK(s.port == 80);
    CHECK(s.headers == (Strings{"A: 1", "B: 2"}));
    CHECK(s.limit == std::optional<int>(5));
}

TEST_CASE("short options take values attached or after a space") {
    Serve s;
    parse(s.parser, {"-p", "81", "-HX: 1"});
    CHECK(s.port == 81);
    CHECK(s.headers == (Strings{"X: 1"}));
}

TEST_CASE("short flags combine, and a value takes the rest of the group") {
    Serve s;
    parse(s.parser, {"-vllp8080"});
    CHECK(s.verbose);
    CHECK(s.level == 2);
    CHECK(s.port == 8080);

    SUBCASE("or the next argument") {
        Serve t;
        parse(t.parser, {"-vp", "90"});
        CHECK(t.verbose);
        CHECK(t.port == 90);
    }
}

TEST_CASE("a value is the next argument even when it starts with -") {
    Serve s;
    parse(s.parser, {"--port", "-1", "--header", "--verbose"});
    CHECK(s.port == -1);
    CHECK(s.headers == (Strings{"--verbose"}));
    CHECK(!s.verbose);
}

TEST_CASE("the last value of an option wins; vectors keep every one") {
    Serve s;
    s.headers = {"default"};
    parse(s.parser, {"--port", "1", "-p", "2", "-H", "a", "--port=3", "-H", "b"});
    CHECK(s.port == 3);
    CHECK(s.headers == (Strings{"default", "a", "b"}));  // after what the vector held
}

TEST_CASE("counters count") {
    Serve s;
    parse(s.parser, {"-l", "--level", "-ll"});
    CHECK(s.level == 4);
}

TEST_CASE("options and positional arguments mix in any order") {
    Serve s;
    parse(s.parser, {"-v", "public", "--port", "1"});
    CHECK(s.root == "public");
    CHECK(s.verbose);
    CHECK(s.port == 1);
}

TEST_CASE("after --, everything is positional") {
    Serve s;
    parse(s.parser, {"--", "--port"});
    CHECK(s.root == "--port");
    CHECK(s.port == 8080);
}

TEST_CASE("- alone is a positional argument") {
    Serve s;
    parse(s.parser, {"-"});
    CHECK(s.root == "-");
}

TEST_CASE("negative numbers are positional, unless a digit is an option") {
    args::Parser parser("calc");
    double a = 0;
    double b = 0;
    parser.positional(a, "A");
    parser.positional(b, "B");
    parse(parser, {"-5", "-0.5"});
    CHECK(a == -5.0);
    CHECK(b == -0.5);

    SUBCASE("with -1 defined, -1 is that option") {
        args::Parser other("calc");
        bool one = false;
        double x = 0;
        other.flag(one, "--one", "-1");
        other.positional(x, "X").optional();
        parse(other, {"-1"});
        CHECK(one);
        CHECK(x == 0.0);
    }
}

TEST_CASE("positional arguments in order, and the last can take the rest") {
    args::Parser parser("cp");
    std::filesystem::path target;
    Strings sources;
    std::string mode = "copy";
    parser.option(mode, "--mode");
    parser.positional(target, "TARGET");
    parser.positional(sources, "SOURCES");
    parse(parser, {"out", "a", "--mode", "move", "b", "c"});
    CHECK(target == std::filesystem::path("out"));
    CHECK(sources == (Strings{"a", "b", "c"}));
    CHECK(mode == "move");
}

TEST_CASE("negatable flags") {
    args::Parser parser("app");
    bool color = true;
    parser.flag(color, "--color").negatable();
    parse(parser, {"--no-color"});
    CHECK(!color);
    parse(parser, {"--color"});
    CHECK(color);
}

TEST_CASE("choices store the value each word maps to") {
    args::Parser parser("app");
    Mode mode = Mode::fast;
    std::vector<Mode> modes;
    parser.option(mode, "--mode").choices({{"fast", Mode::fast}, {"small", Mode::small}});
    parser.option(modes, "--also").choices({{"fast", Mode::fast}, {"small", Mode::small}});
    parse(parser, {"--mode", "small", "--also", "fast", "--also", "small"});
    CHECK(mode == Mode::small);
    CHECK(modes == (std::vector<Mode>{Mode::fast, Mode::small}));
    CHECK(support::usage_error(parser, {"--mode", "quick"}) == "--mode: \"quick\" isn't one of fast, small");
}

TEST_CASE("options of other types: bool, char, float, path") {
    args::Parser parser("app");
    bool cache = true;
    char separator = ',';
    float ratio = 1;
    std::filesystem::path out;
    parser.option(cache, "--cache");
    parser.option(separator, "--separator");
    parser.option(ratio, "--ratio");
    parser.option(out, "--out", "-o");
    parse(parser, {"--cache", "no", "--separator", ";", "--ratio", "0.5", "-o", "build/x"});
    CHECK(!cache);
    CHECK(separator == ';');
    CHECK(ratio == 0.5f);
    CHECK(out == std::filesystem::path("build/x"));
}

// Definition errors (docs/spec.md §3.4): bugs in how a program defines its arguments throw std::logic_error.

#include <cpm/args.hpp>
#include <cpm/test.hpp>

#include <stdexcept>
#include <string>
#include <vector>

#include "support/helpers.hpp"

namespace args = cpm::args;

namespace {

enum class Mode { fast, small };

}  // namespace

TEST_CASE("names must be valid, with a long one") {
    args::Parser parser("app");
    int n = 0;
    CHECK_THROWS_WITH(parser.option(n, "port"), "cpm::args: \"port\" isn't an option name: use --name or -n");
    CHECK_THROWS_AS(parser.option(n, "--Port"), std::logic_error);
    CHECK_THROWS_AS(parser.option(n, "-pp"), std::logic_error);
    CHECK_THROWS_AS(parser.option(n, "--"), std::logic_error);
    CHECK_THROWS_WITH(parser.option(n, "-p"), "cpm::args: an option needs a long name, such as --port");
    CHECK_THROWS_AS(parser.command("Build"), std::logic_error);
    CHECK_THROWS_AS(parser.positional(n, "-N"), std::logic_error);
}

TEST_CASE("names are used once in a command, its ancestors and its descendants") {
    args::Parser parser("app");
    bool a = false;
    bool b = false;
    parser.flag(a, "--all", "-a");
    CHECK_THROWS_WITH(parser.flag(b, "--all"), "cpm::args: --all is already defined");
    CHECK_THROWS_WITH(parser.flag(b, "--both", "-a"), "cpm::args: -a is already defined");
    CHECK_THROWS_AS(parser.flag(b, "--both", "--both"), std::logic_error);

    args::Command& build = parser.command("build");
    CHECK_THROWS_AS(build.flag(b, "--all"), std::logic_error);  // an ancestor has it
    build.flag(b, "--brief", "-b");
    CHECK_THROWS_AS(parser.flag(a, "--brief"), std::logic_error);  // a descendant has it
    CHECK_THROWS_AS(parser.command("build"), std::logic_error);

    args::Command& test = parser.command("test");
    bool c = false;
    test.flag(c, "--brief", "-b");  // siblings may share names
}

TEST_CASE("positional arguments and subcommands don't mix") {
    int n = 0;
    {
        args::Parser parser("app");
        parser.positional(n, "N");
        CHECK_THROWS_AS(parser.command("build"), std::logic_error);
    }
    {
        args::Parser parser("app");
        parser.command("build");
        CHECK_THROWS_AS(parser.positional(n, "N"), std::logic_error);
    }
}

TEST_CASE("nothing comes after a positional that takes the rest") {
    args::Parser parser("app");
    std::vector<std::string> files;
    std::string last;
    parser.positional(files, "FILES");
    CHECK_THROWS_AS(parser.positional(last, "LAST"), std::logic_error);
}

TEST_CASE("required positional arguments come first, as parse checks") {
    args::Parser parser("app");
    std::string a;
    std::string b;
    parser.positional(a, "A").optional();
    parser.positional(b, "B");
    CHECK_THROWS_WITH(support::parse(parser, {}), "cpm::args: the required B comes after the optional A");

    SUBCASE("unless they're optional too, whenever .optional() comes") {
        args::Parser later("app");
        auto first = later.positional(a, "A");
        auto second = later.positional(b, "B");
        first.optional();
        second.optional();
        support::parse(later, {});
    }
}

TEST_CASE("settings that don't fit their argument") {
    args::Parser parser("app");
    int n = 0;
    bool flag = false;
    int count = 0;
    Mode mode = Mode::fast;
    CHECK_THROWS_WITH(parser.option(n, "--number").negatable(), "cpm::args: --number isn't a flag, so it can't be negatable");
    CHECK_THROWS_WITH(parser.option(n, "--other").optional(),
                      "cpm::args: --other is an option, and options are always optional");
    CHECK_THROWS_WITH(parser.flag(flag, "--flag").choices({{"x", true}}),
                      "cpm::args: --flag is a flag or counter and takes no choices");
    CHECK_THROWS_AS(parser.counter(count, "--count").choices({{"x", 1}}), std::logic_error);
    CHECK_THROWS_WITH(parser.option(mode, "--mode").choices({{"fast", Mode::fast}, {"fast", Mode::small}}),
                      "cpm::args: --mode has the choice \"fast\" twice");
}

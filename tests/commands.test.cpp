// Subcommands (docs/spec.md §5.1): selection, nesting, inherited options and optional subcommands.

#include <cpm/args.hpp>
#include <cpm/test.hpp>

#include <string>
#include <vector>

#include "support/helpers.hpp"

namespace args = cpm::args;
using support::parse;
using Strings = std::vector<std::string>;

namespace {

// cpm [--verbose] (build [--release] | test [FILTER] | cache (clean [--all] | show))
struct Cpm {
    args::Parser parser{"cpm", "A package manager"};
    bool verbose = false;
    bool release = false;
    std::string filter;
    bool all = false;
    args::Command& build = parser.command("build", "Compiles the project");
    args::Command& test = parser.command("test", "Runs the tests");
    args::Command& cache = parser.command("cache", "Manages the cache");
    args::Command& clean = cache.command("clean", "Empties it");
    args::Command& show = cache.command("show", "Shows what is in it");

    Cpm() {
        parser.flag(verbose, "--verbose", "-v");
        build.flag(release, "--release", "-r");
        test.positional(filter, "FILTER").optional();
        clean.flag(all, "--all");
    }
};

}  // namespace

TEST_CASE("a command's name selects it, and its arguments follow") {
    Cpm c;
    const args::Outcome outcome = parse(c.parser, {"build", "--release"});
    CHECK(outcome.command == &c.build);
    CHECK(outcome.path == (Strings{"build"}));
    CHECK(c.release);
    CHECK(c.build.path() == "cpm build");
}

TEST_CASE("commands nest") {
    Cpm c;
    const args::Outcome outcome = parse(c.parser, {"cache", "clean", "--all"});
    CHECK(outcome.command == &c.clean);
    CHECK(outcome.path == (Strings{"cache", "clean"}));
    CHECK(c.all);
    CHECK(c.clean.path() == "cpm cache clean");
}

TEST_CASE("ancestors' options work before or after a command's name") {
    Cpm before;
    parse(before.parser, {"-v", "build"});
    CHECK(before.verbose);
    Cpm after;
    parse(after.parser, {"cache", "clean", "--verbose"});
    CHECK(after.verbose);
    Cpm grouped;
    parse(grouped.parser, {"build", "-rv"});
    CHECK(grouped.verbose);
    CHECK(grouped.release);
}

TEST_CASE("a command's options don't work before its name") {
    Cpm c;
    CHECK(support::usage_error(c.parser, {"--release", "build"}) == "unknown option --release");
}

TEST_CASE("positional arguments belong to the command selected") {
    Cpm c;
    parse(c.parser, {"test", "json"});
    CHECK(c.filter == "json");
}

TEST_CASE("a command with subcommands needs one, unless it is optional") {
    Cpm c;
    CHECK(support::usage_error(c.parser, {}) == "missing a command: build, test or cache");
    CHECK(support::usage_error(c.parser, {"cache"}) == "missing a command: clean or show");
    c.cache.subcommand_optional();
    CHECK(parse(c.parser, {"cache"}).command == &c.cache);
}

TEST_CASE("after --, even a command's name is positional") {
    Cpm c;
    CHECK(support::usage_error(c.parser, {"--", "build"}) == "unexpected argument \"build\"");
    parse(c.parser, {"test", "--", "build"});
    CHECK(c.filter == "build");
}

// Usage errors (docs/spec.md §7): every message, suggestions, and help winning over errors.

#include <cpm/args.hpp>
#include <cpm/test.hpp>

#include <string>
#include <vector>

#include "support/helpers.hpp"

namespace args = cpm::args;
using support::parse;
using support::usage_error;

namespace {

struct Serve {
    args::Parser parser{"serve"};
    int port = 0;
    bool verbose = false;
    std::string root;
    std::string name;

    Serve() {
        parser.version("1.0");
        parser.option(port, "--port", "-p");
        parser.flag(verbose, "--verbose", "-v");
        parser.option(name, "--name").required();
        parser.positional(root, "ROOT");
    }
};

}  // namespace

TEST_CASE("unknown options, with suggestions") {
    Serve s;
    CHECK(usage_error(s.parser, {"--prot"}) == "unknown option --prot (did you mean --port?)");
    CHECK(usage_error(s.parser, {"--verbos"}) == "unknown option --verbos (did you mean --verbose?)");
    CHECK(usage_error(s.parser, {"--versoin"}) == "unknown option --versoin (did you mean --version?)");
    CHECK(usage_error(s.parser, {"--colour"}) == "unknown option --colour");
    CHECK(usage_error(s.parser, {"-x"}) == "unknown option -x");
    // -verbose: -v is a flag, but no -e follows it, and --verbose was surely meant.
    CHECK(usage_error(s.parser, {"-verbose"}) == "unknown option -verbose (did you mean --verbose?)");
    // -port is -p with the value "ort", as POSIX reads it.
    CHECK(usage_error(s.parser, {"-port"}) == "-p: \"ort\" isn't an integer");
}

TEST_CASE("values that are missing or don't fit") {
    Serve s;
    CHECK(usage_error(s.parser, {"--port"}) == "--port needs a value");
    CHECK(usage_error(s.parser, {"-p"}) == "-p needs a value");
    CHECK(usage_error(s.parser, {"--port", "abc"}) == "--port: \"abc\" isn't an integer");
    CHECK(usage_error(s.parser, {"-pabc"}) == "-p: \"abc\" isn't an integer");
    CHECK(usage_error(s.parser, {"--verbose=yes"}) == "--verbose doesn't take a value");
}

TEST_CASE("what is required, and what is too much") {
    Serve s;
    CHECK(usage_error(s.parser, {"root"}) == "--name is required");
    CHECK(usage_error(s.parser, {"--name", "n"}) == "missing ROOT");
    CHECK(usage_error(s.parser, {"--name", "n", "root", "extra"}) == "unexpected argument \"extra\"");
}

TEST_CASE("unknown commands, with suggestions") {
    args::Parser parser("cpm");
    parser.command("build");
    parser.command("bench");
    parser.command("test");
    CHECK(usage_error(parser, {"biuld"}) == "unknown command \"biuld\" (did you mean build?)");
    CHECK(usage_error(parser, {"bxnch"}) == "unknown command \"bxnch\" (did you mean bench?)");
    CHECK(usage_error(parser, {"deploy"}) == "unknown command \"deploy\"");
}

TEST_CASE("usage errors say which command's usage applies") {
    args::Parser parser("cpm");
    args::Command& build = parser.command("build");
    try {
        parse(parser, {"build", "--nope"});
        FAIL("expected a usage error");
    } catch (const args::UsageError& error) {
        CHECK(&error.command() == &build);
    }
}

TEST_CASE("help and the version win, even over errors") {
    Serve s;
    CHECK(parse(s.parser, {"--help"}).action == args::Outcome::Action::help);
    CHECK(parse(s.parser, {"-h", "--port", "x"}).action == args::Outcome::Action::help);
    CHECK(parse(s.parser, {"--port", "x", "--help"}).action == args::Outcome::Action::help);
    CHECK(parse(s.parser, {"--bad", "-h"}).action == args::Outcome::Action::help);
    CHECK(parse(s.parser, {"-vV"}).action == args::Outcome::Action::version);
    CHECK(parse(s.parser, {"--version"}).action == args::Outcome::Action::version);
    CHECK(usage_error(s.parser, {"--bad", "--", "--help"}) == "unknown option --bad");  // after --, it's not help

    SUBCASE("help is for the command selected") {
        args::Parser parser("cpm");
        args::Command& build = parser.command("build");
        const args::Outcome outcome = parse(parser, {"build", "--help"});
        CHECK(outcome.action == args::Outcome::Action::help);
        CHECK(outcome.command == &build);
    }
}

TEST_CASE("a program can take -h and -V for itself") {
    args::Parser parser("ssh");
    parser.version("2.0");
    std::string host;
    bool verbose_version = false;
    parser.option(host, "--host", "-h");
    parser.flag(verbose_version, "--very", "-V");
    parse(parser, {"-h", "example.com", "-V"});
    CHECK(host == "example.com");
    CHECK(verbose_version);
    CHECK(parse(parser, {"--help"}).action == args::Outcome::Action::help);
    CHECK(parse(parser, {"--version"}).action == args::Outcome::Action::version);
}

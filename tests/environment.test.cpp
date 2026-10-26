// Environment variables (docs/spec.md §6).

#include <cpm/args.hpp>
#include <cpm/test.hpp>

#include <string>

#include "support/helpers.hpp"

namespace args = cpm::args;
using support::environment;
using support::parse;

namespace {

struct App {
    args::Parser parser{"app"};
    int port = 8080;
    bool debug = false;
    int verbosity = 0;
    std::string token;

    App() {
        parser.option(port, "--port").env("APP_PORT");
        parser.flag(debug, "--debug").env("APP_DEBUG");
        parser.counter(verbosity, "--verbose", "-v").env("APP_VERBOSITY");
        parser.option(token, "--token").env("APP_TOKEN").required();
    }
};

}  // namespace

TEST_CASE("an option not given takes its environment variable") {
    App a;
    parse(a.parser, {}, environment({{"APP_PORT", "9000"}, {"APP_DEBUG", "yes"}, {"APP_VERBOSITY", "2"}, {"APP_TOKEN", "t"}}));
    CHECK(a.port == 9000);
    CHECK(a.debug);
    CHECK(a.verbosity == 2);
    CHECK(a.token == "t");
}

TEST_CASE("the command line wins over the environment") {
    App a;
    parse(a.parser, {"--port", "1", "--token", "cli"}, environment({{"APP_PORT", "9000"}, {"APP_TOKEN", "env"}}));
    CHECK(a.port == 1);
    CHECK(a.token == "cli");
}

TEST_CASE("an empty variable counts as not set") {
    App a;
    CHECK(support::usage_error(a.parser, {}, environment({{"APP_TOKEN", ""}})) == "--token is required");
    parse(a.parser, {}, environment({{"APP_TOKEN", "x"}, {"APP_PORT", ""}}));
    CHECK(a.port == 8080);
}

TEST_CASE("errors in variables name the variable") {
    App a;
    CHECK(support::usage_error(a.parser, {"--token", "t"}, environment({{"APP_PORT", "eighty"}})) ==
          "APP_PORT: \"eighty\" isn't an integer");
    CHECK(support::usage_error(a.parser, {"--token", "t"}, environment({{"APP_DEBUG", "maybe"}})) ==
          "APP_DEBUG: \"maybe\" isn't true or false (or yes, no, on, off, 1, 0)");
}

TEST_CASE("the process environment is the default") {
    args::Parser parser("app");
    std::string path;
    parser.option(path, "--path").env("PATH");  // set on every platform CI runs on
    const std::vector<std::string_view> none;
    parser.parse(none);
    CHECK(!path.empty());
}

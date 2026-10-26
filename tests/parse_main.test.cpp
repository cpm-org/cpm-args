// parse_main (docs/spec.md §7): what main() gets back, and what goes to each stream.

#include <cpm/args.hpp>
#include <cpm/test.hpp>

#include <optional>
#include <sstream>
#include <string>

namespace args = cpm::args;

namespace {

struct Serve {
    args::Parser parser{"serve", "Serves a directory"};
    int port = 8080;
    std::string root = ".";
    std::ostringstream out;
    std::ostringstream err;

    Serve() {
        parser.version("1.4.0");
        parser.option(port, "--port", "-p");
        parser.positional(root, "ROOT").optional();
    }

    template <std::size_t N>
    std::optional<int> run(const char* (&argv)[N]) {
        return parser.parse_main(static_cast<int>(N), argv, out, err);
    }
};

}  // namespace

TEST_CASE("a command line that fits lets the program go on") {
    Serve s;
    const char* argv[] = {"serve", "--port", "81", "www"};
    CHECK(s.run(argv) == std::nullopt);
    CHECK(s.port == 81);
    CHECK(s.root == "www");
    CHECK(s.out.str().empty());
    CHECK(s.err.str().empty());
}

TEST_CASE("help goes to out, and main returns 0") {
    Serve s;
    const char* argv[] = {"serve", "--help"};
    CHECK(s.run(argv) == std::optional<int>(0));
    CHECK(s.out.str() == s.parser.help_text());
    CHECK(s.err.str().empty());
}

TEST_CASE("the version goes to out") {
    Serve s;
    const char* argv[] = {"serve", "-V"};
    CHECK(s.run(argv) == std::optional<int>(0));
    CHECK(s.out.str() == "serve 1.4.0\n");
}

TEST_CASE("usage errors go to err, and main returns 2") {
    Serve s;
    const char* argv[] = {"serve", "--prot", "80"};
    CHECK(s.run(argv) == std::optional<int>(2));
    CHECK(s.out.str().empty());
    CHECK(s.err.str() ==
          "error: unknown option --prot (did you mean --port?)\n"
          "\n"
          "Usage: serve [OPTIONS] [ROOT]\n"
          "\n"
          "For more information, try 'serve --help'.\n");
}

TEST_CASE("errors in a subcommand show its usage") {
    args::Parser parser("cpm");
    parser.command("build", "Compiles");
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"cpm", "build", "extra"};
    CHECK(parser.parse_main(3, argv, out, err) == std::optional<int>(2));
    CHECK(err.str() ==
          "error: unexpected argument \"extra\"\n"
          "\n"
          "Usage: cpm build [OPTIONS]\n"
          "\n"
          "For more information, try 'cpm build --help'.\n");
}

TEST_CASE("outcome() says which command parse_main selected") {
    args::Parser parser("cpm");
    args::Command& build = parser.command("build");
    parser.command("test");
    CHECK(parser.outcome().command == &parser);  // before any parse
    std::ostringstream out;
    std::ostringstream err;
    const char* argv[] = {"cpm", "build"};
    CHECK(parser.parse_main(2, argv, out, err) == std::nullopt);
    CHECK(parser.outcome().command == &build);
    CHECK(parser.outcome().action == args::Outcome::Action::run);
}

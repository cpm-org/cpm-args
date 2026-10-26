// Help (docs/spec.md §8): whole texts, compared exactly.

#include <cpm/args.hpp>
#include <cpm/test.hpp>

#include <filesystem>
#include <string>
#include <vector>

#include "help.hpp"

namespace args = cpm::args;

TEST_CASE("the help of the spec's example") {
    args::Parser parser("serve", "Serves a directory over HTTP");
    parser.version("1.4.0");
    int port = 8080;
    bool verbose = false;
    std::vector<std::string> headers;
    std::filesystem::path root = ".";
    parser.option(port, "--port", "-p").help("port to listen on").value_name("PORT").env("SERVE_PORT");
    parser.flag(verbose, "--verbose", "-v").help("print every request");
    parser.option(headers, "--header", "-H").help("an extra response header");
    parser.positional(root, "ROOT").help("the directory to serve").optional();

    CHECK(parser.help_text() ==
          "Serves a directory over HTTP\n"
          "\n"
          "Usage: serve [OPTIONS] [ROOT]\n"
          "\n"
          "Arguments:\n"
          "  [ROOT]  the directory to serve (default: .)\n"
          "\n"
          "Options:\n"
          "  -p, --port <PORT>      port to listen on (default: 8080) [env: SERVE_PORT]\n"
          "  -v, --verbose          print every request\n"
          "  -H, --header <HEADER>  an extra response header (repeatable)\n"
          "  -h, --help             show this help\n"
          "  -V, --version          show the version\n");
    CHECK(parser.usage_text() == "Usage: serve [OPTIONS] [ROOT]");
}

TEST_CASE("commands, and a subcommand's help with what it inherits") {
    args::Parser parser("cpm", "A package manager");
    parser.version("1.0");
    bool verbose = false;
    bool release = false;
    parser.flag(verbose, "--verbose", "-v").help("print more");
    args::Command& build = parser.command("build", "Compiles the project");
    parser.command("test", "Runs the tests");
    build.flag(release, "--release", "-r").help("optimise");

    CHECK(parser.help_text() ==
          "A package manager\n"
          "\n"
          "Usage: cpm [OPTIONS] <COMMAND>\n"
          "\n"
          "Commands:\n"
          "  build  Compiles the project\n"
          "  test   Runs the tests\n"
          "\n"
          "Options:\n"
          "  -v, --verbose  print more\n"
          "  -h, --help     show this help\n"
          "  -V, --version  show the version\n");
    CHECK(parser.help_text(build) ==
          "Compiles the project\n"
          "\n"
          "Usage: cpm build [OPTIONS]\n"
          "\n"
          "Options:\n"
          "  -r, --release  optimise\n"
          "  -h, --help     show this help\n"
          "\n"
          "Inherited options:\n"
          "  -v, --verbose  print more\n"
          "  -V, --version  show the version\n");

    SUBCASE("an optional subcommand shows in brackets") {
        parser.subcommand_optional();
        CHECK(parser.usage_text() == "Usage: cpm [OPTIONS] [COMMAND]");
    }
}

TEST_CASE("notes, labels and hidden arguments") {
    args::Parser parser("tool");
    std::string mode = "fast";
    int verbose = 0;
    bool color = true;
    int jobs = 4;
    std::string secret;
    std::vector<std::string> files;
    parser.option(mode, "--mode").help("how to work").choices({{"fast", "fast"}, {"small", "small"}});
    parser.counter(verbose, "--verbose", "-v").help("more output");
    parser.flag(color, "--color").negatable().help("color the output");
    parser.option(jobs, "--jobs", "-j").help("parallel jobs").default_text("the number of cores");
    parser.option(secret, "--secret").hidden();
    parser.positional(files, "FILES").help("the inputs").optional();

    CHECK(parser.help_text() ==
          "Usage: tool [OPTIONS] [FILES...]\n"
          "\n"
          "Arguments:\n"
          "  [FILES...]  the inputs\n"
          "\n"
          "Options:\n"
          "      --mode <MODE>  how to work (default: fast) [fast, small]\n"
          "  -v, --verbose      more output (repeatable)\n"
          "      --[no-]color   color the output (default: true)\n"
          "  -j, --jobs <JOBS>  parallel jobs (default: the number of cores)\n"
          "  -h, --help         show this help\n");
}

TEST_CASE("help wraps at the width") {
    args::Parser parser("app");
    std::string name;
    parser.option(name, "--name", "-n").help("the name to greet, which can be as long as you like and wraps");

    parser.width(60);
    CHECK(parser.help_text() ==
          "Usage: app [OPTIONS]\n"
          "\n"
          "Options:\n"
          "  -n, --name <NAME>  the name to greet, which can be as long\n"  // 39 columns: 60 - 21
          "                     as you like and wraps\n"
          "  -h, --help         show this help\n");

    SUBCASE("and a first column too wide puts the help on the next line") {
        parser.width(40);
        CHECK(parser.help_text() ==
              "Usage: app [OPTIONS]\n"
              "\n"
              "Options:\n"
              "  -n, --name <NAME>\n"
              "    the name to greet, which can be as\n"
              "    long as you like and wraps\n"
              "  -h, --help\n"
              "    show this help\n");
    }
}

TEST_CASE("the program's own -h leaves --help alone") {
    args::Parser parser("ssh");
    std::string host;
    parser.option(host, "--host", "-h").help("where to connect");
    CHECK(parser.help_text() ==
          "Usage: ssh [OPTIONS]\n"
          "\n"
          "Options:\n"
          "  -h, --host <HOST>  where to connect\n"
          "      --help         show this help\n");
}

TEST_CASE("wrapping") {
    namespace detail = cpm::args::detail;
    CHECK(detail::wrap("a bb ccc dddd", 6) == (std::vector<std::string>{"a bb", "ccc", "dddd"}));
    CHECK(detail::wrap("averyveryverylongword x", 5) == (std::vector<std::string>{"averyveryverylongword", "x"}));
    CHECK(detail::wrap("  spaced   out  ", 80) == (std::vector<std::string>{"spaced out"}));
    CHECK(detail::wrap("", 10).empty());
}

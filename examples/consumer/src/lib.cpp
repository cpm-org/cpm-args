#include <consumer/options.hpp>

#include <cpm/args.hpp>

std::optional<int> read_options(int argc, const char* const* argv, Options& options, std::ostream& out,
                                std::ostream& err) {
    cpm::args::Parser parser("todo", "Keeps a list of things to do");
    parser.version("0.1.0");
    parser.option(options.data, "--data", "-d").help("the file the list is kept in").env("TODO_DATA");

    cpm::args::Command& add = parser.command("add", "Adds an item");
    add.positional(options.words, "TEXT").help("what to do");
    add.option(options.due, "--due").help("when it is due").value_name("DATE");

    cpm::args::Command& list = parser.command("list", "Shows the items");
    list.flag(options.all, "--all", "-a").help("include the done ones");

    const std::optional<int> exit_code = parser.parse_main(argc, argv, out, err);
    if (!exit_code) options.command = parser.outcome().command->name();
    return exit_code;
}

#pragma once

#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

// What `todo` was asked to do: `todo add TEXT... [--due DATE]` or `todo list [--all]`, with --data anywhere.
struct Options {
    std::string command;  // "add" or "list"
    std::string data = "todo.txt";
    std::vector<std::string> words;
    std::optional<std::string> due;
    bool all = false;
};

// Reads the command line into `options`. Returns the exit code when main should stop (help, version or an error,
// written to `out` or `err`), nullopt to go on.
std::optional<int> read_options(int argc, const char* const* argv, Options& options, std::ostream& out,
                                std::ostream& err);

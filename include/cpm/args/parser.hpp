// Parsing a command line (docs/spec.md §7) and help (§8).
#pragma once

#include <cpm/args/command.hpp>
#include <cpm/args/environment.hpp>
#include <cpm/args/error.hpp>

#include <cstddef>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace cpm::args {

struct Outcome {
    enum class Action { run, help, version };
    Action action = Action::run;      // run: the program goes on; help or version was asked for
    const Command* command = nullptr;  // the deepest command selected
    std::vector<std::string> path;    // the names of the commands selected, outermost first
};

// The root command: the program's name, about text and version, and the functions that parse.
class Parser : public Command {
public:
    explicit Parser(std::string name, std::string about = "");

    // Adds -V and --version (unless the program uses those names).
    Parser& version(std::string version);
    const std::string& version() const noexcept { return version_; }

    // The column help wraps at. Default: 80.
    Parser& width(std::size_t columns) noexcept;
    std::size_t width() const noexcept { return width_; }

    // Parses the arguments after argv[0], with the process's environment.
    Outcome parse(int argc, const char* const* argv);

    // Parses `args` (without the program's name), filling in the variables. Throws UsageError for a command line that
    // doesn't fit, and std::logic_error for arguments defined wrongly.
    Outcome parse(std::span<const std::string_view> args, const Environment& environment = system_environment());

    // For main(): help and the version go to `out` (returns 0), usage errors to `err` (returns 2). nullopt: go on, and
    // outcome() says which command was selected.
    std::optional<int> parse_main(int argc, const char* const* argv, std::ostream& out = std::cout,
                                  std::ostream& err = std::cerr);

    // The outcome of the last parse that didn't throw: which command was selected, and whether help or the version
    // was asked for. Before any, the root and Action::run.
    const Outcome& outcome() const noexcept { return outcome_; }

    // The help of a command (this one by default), ending in a line break.
    std::string help_text(const Command& command) const;
    std::string help_text() const { return help_text(*this); }

    // "Usage: serve [OPTIONS] [ROOT]", without a line break.
    std::string usage_text(const Command& command) const;
    std::string usage_text() const { return usage_text(*this); }

private:
    std::string version_;
    std::size_t width_ = 80;
    Outcome outcome_;
};

}  // namespace cpm::args

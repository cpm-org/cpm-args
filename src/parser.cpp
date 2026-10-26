#include <cpm/args/parser.hpp>

#include <ostream>
#include <utility>

#include "help.hpp"
#include "matcher.hpp"

namespace cpm::args {

Parser::Parser(std::string name, std::string about) : Command(std::move(name), std::move(about), nullptr) {
    outcome_.command = this;
}

Parser& Parser::version(std::string version) {
    version_ = std::move(version);
    return *this;
}

Parser& Parser::width(std::size_t columns) noexcept {
    width_ = columns;
    return *this;
}

Outcome Parser::parse(int argc, const char* const* argv) {
    std::vector<std::string_view> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
    return parse(args);
}

Outcome Parser::parse(std::span<const std::string_view> args, const Environment& environment) {
    validate();
    outcome_ = detail::match(*this, args, environment, !version_.empty());
    return outcome_;
}

std::optional<int> Parser::parse_main(int argc, const char* const* argv, std::ostream& out, std::ostream& err) {
    try {
        const Outcome outcome = parse(argc, argv);
        switch (outcome.action) {
            case Outcome::Action::help: out << help_text(*outcome.command); return 0;
            case Outcome::Action::version: out << name() << ' ' << version_ << '\n'; return 0;
            case Outcome::Action::run: return std::nullopt;
        }
    } catch (const UsageError& error) {
        err << "error: " << error.what() << "\n\n"
            << usage_text(error.command()) << "\n\n"
            << "For more information, try '" << error.command().path() << " --help'.\n";
        return 2;
    }
    return std::nullopt;
}

std::string Parser::help_text(const Command& command) const {
    return detail::help_text(command, width_, !version_.empty());
}

std::string Parser::usage_text(const Command& command) const { return detail::usage_line(command); }

}  // namespace cpm::args

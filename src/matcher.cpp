#include "matcher.hpp"

#include <format>
#include <set>
#include <string>
#include <vector>

#include "suggest.hpp"
#include "tokens.hpp"

namespace cpm::args::detail {

namespace {

// "build", "build or test", "build, test or run"
std::string either(const std::vector<std::string>& names) {
    std::string text;
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i > 0) text += i + 1 == names.size() ? " or " : ", ";
        text += names[i];
    }
    return text;
}

class Matcher {
public:
    Matcher(const Command& root, std::span<const std::string_view> args, const Environment& environment,
            bool has_version)
        : args_(args), environment_(environment), has_version_(has_version), current_(&root) {}

    Outcome run() {
        try {
            while (action_ == Outcome::Action::run && index_ < args_.size()) step(args_[index_++]);
            if (action_ == Outcome::Action::run) finish();
        } catch (const UsageError&) {
            if (asks_for_help_or_version_later()) return outcome();
            throw;
        }
        return outcome();
    }

private:
    Outcome outcome() const { return {action_, current_, path_}; }

    [[noreturn]] void fail(const std::string& message) const { throw UsageError(message, *current_); }

    bool is_help(std::string_view long_name) const { return long_name == "help" && current_->find_long("help") == nullptr; }
    bool is_help(char short_name) const { return short_name == 'h' && current_->find_short('h') == nullptr; }
    bool is_version(std::string_view long_name) const {
        return has_version_ && long_name == "version" && current_->find_long("version") == nullptr;
    }
    bool is_version(char short_name) const {
        return has_version_ && short_name == 'V' && current_->find_short('V') == nullptr;
    }

    void step(std::string_view argument) {
        if (only_positionals_) return on_positional(argument);  // after --, not even a command's name
        const Token token = classify(argument);
        switch (token.kind) {
            case Token::Kind::separator: only_positionals_ = true; return;
            case Token::Kind::long_option: return on_long(token);
            case Token::Kind::short_group: return on_short(token, argument);
            case Token::Kind::plain: return on_plain(argument);
        }
    }

    void on_long(const Token& token) {
        const std::string typed = "--" + std::string(token.name);
        if (is_help(token.name)) {
            action_ = Outcome::Action::help;
            return;
        }
        if (is_version(token.name)) {
            action_ = Outcome::Action::version;
            return;
        }
        const Argument* argument = current_->find_long(token.name);
        bool negation = false;
        if (argument == nullptr && token.name.starts_with("no-")) {
            const Argument* negated = current_->find_long(token.name.substr(3));
            if (negated != nullptr && negated->is_negatable()) {
                argument = negated;
                negation = true;
            }
        }
        if (argument == nullptr) fail(unknown_option(typed));
        if (argument->kind() == Kind::flag || argument->kind() == Kind::counter) {
            if (token.value) fail(typed + " doesn't take a value");
            argument->binding().signal(!negation);
        } else {
            assign(*argument, token.value ? *token.value : next_value(typed), typed);
        }
        given_.insert(argument);
    }

    void on_short(const Token& token, std::string_view argument) {
        const std::string_view letters = token.name;
        if (letters.front() >= '0' && letters.front() <= '9' && !has_digit_options() && is_number(argument)) {
            return on_plain(argument);
        }
        for (std::size_t i = 0; i < letters.size(); ++i) {
            const char letter = letters[i];
            const std::string typed = std::string("-") + letter;
            if (is_help(letter)) {
                action_ = Outcome::Action::help;
                return;
            }
            if (is_version(letter)) {
                action_ = Outcome::Action::version;
                return;
            }
            const Argument* option = current_->find_short(letter);
            if (option == nullptr) {
                // "-port" for "--port" is a common slip.
                const bool long_meant = letters.size() > 1 && current_->find_long(letters) != nullptr;
                fail(long_meant ? std::format("unknown option {} (did you mean --{}?)", argument, letters)
                                : std::format("unknown option {}", typed));
            }
            given_.insert(option);
            if (option->kind() == Kind::flag || option->kind() == Kind::counter) {
                option->binding().signal(true);
                continue;
            }
            const std::string_view rest = letters.substr(i + 1);
            assign(*option, rest.empty() ? next_value(typed) : rest, typed);
            return;
        }
    }

    void on_plain(std::string_view argument) {
        if (!current_->commands().empty()) {
            if (const Command* sub = current_->find_command(argument)) {
                current_ = sub;
                path_.emplace_back(sub->name());
                return;
            }
            std::vector<std::string> names;
            for (const auto& sub : current_->commands()) names.push_back(sub->name());
            const std::optional<std::string> meant = closest(argument, names);
            fail(meant ? std::format("unknown command \"{}\" (did you mean {}?)", argument, *meant)
                       : std::format("unknown command \"{}\"", argument));
        }
        on_positional(argument);
    }

    void on_positional(std::string_view argument) {
        const auto& positionals = current_->positionals();
        if (positional_ >= positionals.size()) fail(std::format("unexpected argument \"{}\"", argument));
        const Argument& positional = *positionals[positional_];
        assign(positional, argument, positional.display_name());
        given_.insert(&positional);
        if (positional.kind() != Kind::list) ++positional_;
    }

    std::string_view next_value(const std::string& typed) {
        if (index_ >= args_.size()) fail(typed + " needs a value");
        return args_[index_++];
    }

    void assign(const Argument& argument, std::string_view text, std::string_view source) const {
        try {
            argument.binding().assign(text);
        } catch (const ValueError& error) {
            fail(std::format("{}: \"{}\" {}", source, text, error.what()));
        }
    }

    bool has_digit_options() const {
        for (char digit = '0'; digit <= '9'; ++digit) {
            if (current_->find_short(digit) != nullptr) return true;
        }
        return false;
    }

    std::string unknown_option(const std::string& typed) const {
        std::vector<std::string> names;
        for (const Command* c = current_; c != nullptr; c = c->parent()) {
            for (const auto& option : c->options()) {
                for (const std::string& name : option->long_names()) {
                    names.push_back("--" + name);
                    if (option->is_negatable()) names.push_back("--no-" + name);
                }
            }
        }
        if (current_->find_long("help") == nullptr) names.emplace_back("--help");
        if (has_version_ && current_->find_long("version") == nullptr) names.emplace_back("--version");
        const std::optional<std::string> meant = closest(typed, names);
        return meant ? std::format("unknown option {} (did you mean {}?)", typed, *meant)
                     : std::format("unknown option {}", typed);
    }

    // After the loop: environment variables for what wasn't given, then what is required.
    void finish() {
        if (current_->needs_subcommand()) {
            std::vector<std::string> names;
            for (const auto& sub : current_->commands()) names.push_back(sub->name());
            fail("missing a command: " + either(names));
        }
        std::vector<const Command*> chain;
        for (const Command* c = current_; c != nullptr; c = c->parent()) chain.insert(chain.begin(), c);
        for (const Command* c : chain) {
            for (const auto& option : c->options()) {
                if (given_.contains(option.get()) || option->env().empty()) continue;
                const std::optional<std::string> value = environment_ ? environment_(option->env()) : std::nullopt;
                if (!value || value->empty()) continue;
                assign(*option, *value, option->env());
                given_.insert(option.get());
            }
        }
        for (const Command* c : chain) {
            for (const auto& option : c->options()) {
                if (option->is_required() && !given_.contains(option.get())) fail(option->display_name() + " is required");
            }
        }
        for (const auto& positional : current_->positionals()) {
            if (!positional->is_optional() && !given_.contains(positional.get())) fail("missing " + positional->display_name());
        }
    }

    // After an error: whether the rest of the command line (before any --) asks for help or the version, which win.
    bool asks_for_help_or_version_later() {
        for (std::size_t i = index_; i < args_.size() && args_[i] != "--"; ++i) {
            const std::string_view argument = args_[i];
            if ((argument == "--help" && is_help("help")) || (argument == "-h" && is_help('h'))) {
                action_ = Outcome::Action::help;
                return true;
            }
            if ((argument == "--version" && is_version("version")) || (argument == "-V" && is_version('V'))) {
                action_ = Outcome::Action::version;
                return true;
            }
        }
        return false;
    }

    std::span<const std::string_view> args_;
    const Environment& environment_;
    bool has_version_;
    const Command* current_;
    std::vector<std::string> path_;
    std::set<const Argument*> given_;
    std::size_t index_ = 0;
    std::size_t positional_ = 0;
    bool only_positionals_ = false;
    Outcome::Action action_ = Outcome::Action::run;
};

}  // namespace

Outcome match(const Command& root, std::span<const std::string_view> args, const Environment& environment,
              bool has_version) {
    return Matcher(root, args, environment, has_version).run();
}

}  // namespace cpm::args::detail

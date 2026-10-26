// Commands: arguments and subcommands (docs/spec.md §3, §5.1).
#pragma once

#include <cpm/args/argument.hpp>
#include <cpm/args/binding.hpp>

#include <concepts>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace cpm::args {

// A command with its arguments and subcommands. Parser is the root command; command() adds subcommands. Defining an
// argument wrongly throws std::logic_error at once (docs/spec.md §3.4).
class Command {
public:
    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;
    virtual ~Command();

    const std::string& name() const noexcept { return name_; }
    const std::string& about() const noexcept { return about_; }
    const Command* parent() const noexcept { return parent_; }
    // The names from the root's down to this one's, separated by spaces: "cpm build".
    std::string path() const;

    // An option with a value, read as T; std::vector<T> takes it any number of times, and std::optional<T> is set when
    // it is given. The first name must be long: option(port, "--port", "-p").
    template <class T, class... Names>
        requires(std::convertible_to<const Names&, std::string_view> && ...)
    Definition<detail::element_t<T>> option(T& target, const Names&... names) {
        auto binding = std::make_unique<detail::ValueBinding<T>>(target);
        detail::ChoiceTarget<detail::element_t<T>>* choices = binding.get();
        Argument& argument = add_option(std::move(binding), {std::string_view(names)...});
        return {argument, choices};
    }

    // A switch: true when given.
    template <class... Names>
        requires(std::convertible_to<const Names&, std::string_view> && ...)
    Definition<bool> flag(bool& target, const Names&... names) {
        return {add_option(std::make_unique<detail::FlagBinding>(target), {std::string_view(names)...}), nullptr};
    }

    // One more each time it is given: -vvv is 3.
    template <class... Names>
        requires(std::convertible_to<const Names&, std::string_view> && ...)
    Definition<int> counter(int& target, const Names&... names) {
        return {add_option(std::make_unique<detail::CounterBinding>(target), {std::string_view(names)...}), nullptr};
    }

    // A positional argument, required unless .optional(); std::vector<T> takes the rest.
    template <class T>
    Definition<detail::element_t<T>> positional(T& target, std::string_view display_name) {
        auto binding = std::make_unique<detail::ValueBinding<T>>(target);
        detail::ChoiceTarget<detail::element_t<T>>* choices = binding.get();
        Argument& argument = add_positional(std::move(binding), display_name);
        return {argument, choices};
    }

    // A subcommand. A command with subcommands needs one of them on the command line, unless subcommand_optional().
    Command& command(std::string name, std::string about = "");
    Command& subcommand_optional(bool optional = true) noexcept;
    bool needs_subcommand() const noexcept { return !commands_.empty() && !subcommand_optional_; }

    const std::vector<std::unique_ptr<Argument>>& options() const noexcept { return options_; }
    const std::vector<std::unique_ptr<Argument>>& positionals() const noexcept { return positionals_; }
    const std::vector<std::unique_ptr<Command>>& commands() const noexcept { return commands_; }

    // The option with this long name (without "--") or short name, here or in an ancestor; nullptr if none.
    const Argument* find_long(std::string_view name) const noexcept;
    const Argument* find_short(char name) const noexcept;
    // The direct subcommand with this name; nullptr if none.
    const Command* find_command(std::string_view name) const noexcept;
    const Command& root() const noexcept;

protected:
    Command(std::string name, std::string about, const Command* parent);

    // The rules that depend on settings made after definition: no required positional after an optional one, in this
    // command and its subcommands. Throws std::logic_error.
    void validate() const;

private:
    Argument& add_option(std::unique_ptr<detail::Binding> binding, std::initializer_list<std::string_view> names);
    Argument& add_positional(std::unique_ptr<detail::Binding> binding, std::string_view display_name);
    // Throws std::logic_error when this command, an ancestor or a descendant already uses the name.
    void check_unused(const std::vector<std::string>& long_names, const std::string& short_names) const;
    bool uses_here(std::string_view long_name, char short_name) const noexcept;
    bool used_below(std::string_view long_name, char short_name) const noexcept;

    std::string name_;
    std::string about_;
    const Command* parent_;
    std::vector<std::unique_ptr<Argument>> options_;
    std::vector<std::unique_ptr<Argument>> positionals_;
    std::vector<std::unique_ptr<Command>> commands_;
    bool subcommand_optional_ = false;
};

}  // namespace cpm::args

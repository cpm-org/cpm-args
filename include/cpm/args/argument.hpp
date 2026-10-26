// Arguments and their settings (docs/spec.md §3.2, §3.3).
#pragma once

#include <cpm/args/binding.hpp>

#include <initializer_list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cpm::args {

class Command;

// One flag, counter, option or positional argument, as a command defined it.
class Argument {
public:
    Argument(const Argument&) = delete;
    Argument& operator=(const Argument&) = delete;
    ~Argument();

    // Long names without "--", in the order given; empty for positional arguments.
    const std::vector<std::string>& long_names() const noexcept { return long_names_; }
    // Short names, one letter each.
    const std::string& short_names() const noexcept { return short_names_; }
    bool is_positional() const noexcept { return positional_; }
    detail::Kind kind() const noexcept { return binding_->kind(); }

    // How help and errors name it: "--port", or a positional argument's display name.
    const std::string& display_name() const noexcept { return display_name_; }
    // The placeholder in help: the value name if set, else the long name in uppercase (PORT), or the display name.
    std::string value_name() const;

    const std::string& help() const noexcept { return help_; }
    bool is_required() const noexcept { return required_; }
    bool is_optional() const noexcept { return optional_; }  // positional arguments
    const std::string& env() const noexcept { return env_; }
    bool is_negatable() const noexcept { return negatable_; }
    bool is_hidden() const noexcept { return hidden_; }
    // The default help shows: default_text if set, else the variable's starting value if it can be shown.
    std::optional<std::string> shown_default() const;
    std::vector<std::string> choice_words() const { return binding_->choice_words(); }

    // Settings, usually set through the Definition that defining an argument returns.
    void help(std::string text) { help_ = std::move(text); }
    void value_name(std::string name) { value_name_ = std::move(name); }
    void required() noexcept { required_ = true; }
    // Positional arguments only: may be left out. Optional ones must come after the required ones; parsing checks.
    void optional();
    void env(std::string variable) { env_ = std::move(variable); }
    // Flags only: also defines --no-<name>, which sets false.
    void negatable();
    void hidden() noexcept { hidden_ = true; }
    void default_text(std::string text) { default_text_ = std::move(text); }

    detail::Binding& binding() const noexcept { return *binding_; }

private:
    friend class Command;
    Argument(std::unique_ptr<detail::Binding> binding, std::vector<std::string> long_names, std::string short_names,
             std::string display_name, bool positional);

    std::unique_ptr<detail::Binding> binding_;
    std::vector<std::string> long_names_;
    std::string short_names_;
    std::string display_name_;
    bool positional_;
    std::string help_;
    std::string value_name_;
    std::string env_;
    std::optional<std::string> default_text_;
    bool required_ = false;
    bool optional_ = false;
    bool negatable_ = false;
    bool hidden_ = false;
};

// What defining an argument returns: its settings, chained (docs/spec.md §3.3). E is the value's element type, so
// choices() takes values of the right type.
template <class E>
class Definition {
public:
    Definition(Argument& argument, detail::ChoiceTarget<E>* choices) noexcept
        : argument_(&argument), choices_(choices) {}

    Definition& help(std::string text) {
        argument_->help(std::move(text));
        return *this;
    }
    Definition& value_name(std::string name) {
        argument_->value_name(std::move(name));
        return *this;
    }
    Definition& required() noexcept {
        argument_->required();
        return *this;
    }
    Definition& optional() {
        argument_->optional();
        return *this;
    }
    Definition& env(std::string variable) {
        argument_->env(std::move(variable));
        return *this;
    }
    Definition& negatable() {
        argument_->negatable();
        return *this;
    }
    Definition& hidden() noexcept {
        argument_->hidden();
        return *this;
    }
    Definition& default_text(std::string text) {
        argument_->default_text(std::move(text));
        return *this;
    }

    // Accepts only these words, storing the value each one maps to. Throws std::logic_error for a flag or counter, or
    // a repeated word.
    Definition& choices(std::initializer_list<std::pair<std::string_view, E>> words) {
        if (choices_ == nullptr) {
            throw std::logic_error("cpm::args: " + argument_->display_name() + " is a flag or counter and takes no choices");
        }
        std::vector<std::pair<std::string, E>> list;
        for (const auto& [word, value] : words) {
            for (const auto& [seen, unused] : list) {
                if (seen == word) {
                    throw std::logic_error("cpm::args: " + argument_->display_name() + " has the choice \"" +
                                           std::string(word) + "\" twice");
                }
            }
            list.emplace_back(std::string(word), value);
        }
        choices_->set_choices(std::move(list));
        return *this;
    }

    Argument& argument() const noexcept { return *argument_; }

private:
    Argument* argument_;
    detail::ChoiceTarget<E>* choices_;
};

}  // namespace cpm::args

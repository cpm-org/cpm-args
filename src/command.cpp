#include <cpm/args/command.hpp>

#include <algorithm>
#include <stdexcept>

#include "names.hpp"

namespace cpm::args {

namespace {

[[noreturn]] void misdefined(const std::string& what) { throw std::logic_error("cpm::args: " + what); }

}  // namespace

Command::Command(std::string name, std::string about, const Command* parent)
    : name_(std::move(name)), about_(std::move(about)), parent_(parent) {}

Command::~Command() = default;

std::string Command::path() const { return parent_ == nullptr ? name_ : parent_->path() + " " + name_; }

const Command& Command::root() const noexcept { return parent_ == nullptr ? *this : parent_->root(); }

Command& Command::command(std::string name, std::string about) {
    if (!detail::is_command_name(name)) {
        misdefined("\"" + name + "\" isn't a command name: use lowercase letters, digits and '-', starting with a letter");
    }
    if (find_command(name) != nullptr) misdefined("the command " + path() + " already has a subcommand " + name);
    if (!positionals_.empty()) {
        misdefined("the command " + path() + " has positional arguments, so it can't have subcommands");
    }
    commands_.push_back(std::unique_ptr<Command>(new Command(std::move(name), std::move(about), this)));
    return *commands_.back();
}

Command& Command::subcommand_optional(bool optional) noexcept {
    subcommand_optional_ = optional;
    return *this;
}

Argument& Command::add_option(std::unique_ptr<detail::Binding> binding, std::initializer_list<std::string_view> names) {
    std::vector<std::string> long_names;
    std::string short_names;
    for (std::string_view name : names) {
        if (name.starts_with("--") && detail::is_long_name(name.substr(2))) {
            long_names.emplace_back(name.substr(2));
        } else if (name.size() == 2 && name.front() == '-' && detail::is_short_name(name[1])) {
            short_names += name[1];
        } else {
            misdefined("\"" + std::string(name) + "\" isn't an option name: use --name or -n");
        }
    }
    if (long_names.empty()) misdefined("an option needs a long name, such as --port");
    check_unused(long_names, short_names);
    const std::string display = "--" + long_names.front();
    options_.push_back(std::unique_ptr<Argument>(
        new Argument(std::move(binding), std::move(long_names), std::move(short_names), display, false)));
    return *options_.back();
}

Argument& Command::add_positional(std::unique_ptr<detail::Binding> binding, std::string_view display_name) {
    if (!detail::is_display_name(display_name)) {
        misdefined("\"" + std::string(display_name) + "\" isn't a name for a positional argument");
    }
    if (!commands_.empty()) {
        misdefined("the command " + path() + " has subcommands, so it can't have positional arguments");
    }
    if (!positionals_.empty() && positionals_.back()->kind() == detail::Kind::list) {
        misdefined(std::string(display_name) + " comes after " + positionals_.back()->display_name() +
                   ", which takes all the remaining arguments");
    }
    for (const auto& other : positionals_) {
        if (other->display_name() == display_name) misdefined("the positional argument " + std::string(display_name) + " is defined twice");
    }
    positionals_.push_back(std::unique_ptr<Argument>(
        new Argument(std::move(binding), {}, {}, std::string(display_name), true)));
    return *positionals_.back();
}

bool Command::uses_here(std::string_view long_name, char short_name) const noexcept {
    for (const auto& option : options_) {
        if (!long_name.empty() &&
            std::find(option->long_names().begin(), option->long_names().end(), long_name) != option->long_names().end())
            return true;
        if (short_name != '\0' && option->short_names().find(short_name) != std::string::npos) return true;
    }
    return false;
}

bool Command::used_below(std::string_view long_name, char short_name) const noexcept {
    for (const auto& sub : commands_) {
        if (sub->uses_here(long_name, short_name) || sub->used_below(long_name, short_name)) return true;
    }
    return false;
}

void Command::check_unused(const std::vector<std::string>& long_names, const std::string& short_names) const {
    auto used = [&](std::string_view long_name, char short_name) {
        for (const Command* c = this; c != nullptr; c = c->parent_) {
            if (c->uses_here(long_name, short_name)) return true;
        }
        return used_below(long_name, short_name);
    };
    for (std::size_t i = 0; i < long_names.size(); ++i) {
        const std::string& name = long_names[i];
        if (used(name, '\0') || std::count(long_names.begin(), long_names.end(), name) > 1) {
            misdefined("--" + name + " is already defined");
        }
    }
    for (char name : short_names) {
        if (used({}, name) || std::count(short_names.begin(), short_names.end(), name) > 1) {
            misdefined(std::string("-") + name + " is already defined");
        }
    }
}

const Argument* Command::find_long(std::string_view name) const noexcept {
    for (const Command* c = this; c != nullptr; c = c->parent_) {
        for (const auto& option : c->options_) {
            const auto& names = option->long_names();
            if (std::find(names.begin(), names.end(), name) != names.end()) return option.get();
        }
    }
    return nullptr;
}

const Argument* Command::find_short(char name) const noexcept {
    for (const Command* c = this; c != nullptr; c = c->parent_) {
        for (const auto& option : c->options_) {
            if (option->short_names().find(name) != std::string::npos) return option.get();
        }
    }
    return nullptr;
}

void Command::validate() const {
    const Argument* optional = nullptr;
    for (const auto& positional : positionals_) {
        if (positional->is_optional()) {
            optional = positional.get();
        } else if (optional != nullptr) {
            misdefined("the required " + positional->display_name() + " comes after the optional " +
                       optional->display_name());
        }
    }
    for (const auto& sub : commands_) sub->validate();
}

const Command* Command::find_command(std::string_view name) const noexcept {
    for (const auto& sub : commands_) {
        if (sub->name() == name) return sub.get();
    }
    return nullptr;
}

}  // namespace cpm::args

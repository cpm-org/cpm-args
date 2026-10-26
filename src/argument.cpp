#include <cpm/args/argument.hpp>

#include <cctype>

namespace cpm::args {

Argument::Argument(std::unique_ptr<detail::Binding> binding, std::vector<std::string> long_names,
                   std::string short_names, std::string display_name, bool positional)
    : binding_(std::move(binding)),
      long_names_(std::move(long_names)),
      short_names_(std::move(short_names)),
      display_name_(std::move(display_name)),
      positional_(positional) {}

Argument::~Argument() = default;

std::string Argument::value_name() const {
    if (!value_name_.empty()) return value_name_;
    if (positional_) return display_name_;
    std::string name = long_names_.front();
    for (char& c : name) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return name;
}

std::optional<std::string> Argument::shown_default() const {
    if (default_text_) return default_text_;
    return binding_->shown_default();
}

void Argument::optional() {
    if (!positional_) {
        throw std::logic_error("cpm::args: " + display_name_ + " is an option, and options are always optional");
    }
    optional_ = true;
}

void Argument::negatable() {
    if (kind() != detail::Kind::flag) {
        throw std::logic_error("cpm::args: " + display_name_ + " isn't a flag, so it can't be negatable");
    }
    negatable_ = true;
}

}  // namespace cpm::args

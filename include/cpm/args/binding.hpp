// Where a value goes: the link from an argument to its variable (docs/spec.md §3.2). Internal: use Command's functions.
#pragma once

#include <cpm/args/error.hpp>
#include <cpm/args/reader.hpp>

#include <concepts>
#include <filesystem>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cpm::args::detail {

enum class Kind { flag, counter, value, list };

template <class T>
struct Target {
    using element = T;
    static constexpr Kind kind = Kind::value;
    static constexpr bool optional = false;
};

template <class T, class Allocator>
struct Target<std::vector<T, Allocator>> {
    using element = T;
    static constexpr Kind kind = Kind::list;
    static constexpr bool optional = false;
};

template <class T>
struct Target<std::optional<T>> {
    using element = T;
    static constexpr Kind kind = Kind::value;
    static constexpr bool optional = true;
};

template <class T>
using element_t = typename Target<T>::element;

template <class T>
concept formattable = std::semiregular<std::formatter<T, char>>;

// How help shows a value: its std::format text, a path's text, "true" for true; nothing for false, empty text, or a
// type that can't be formatted.
template <class T>
std::optional<std::string> shown(const T& value) {
    std::string text;
    if constexpr (std::same_as<T, bool>) {
        if (!value) return std::nullopt;
        text = "true";
    } else if constexpr (std::same_as<T, std::filesystem::path>) {
        text = value.string();
    } else if constexpr (formattable<T>) {
        text = std::format("{}", value);
    } else {
        return std::nullopt;
    }
    if (text.empty()) return std::nullopt;
    return text;
}

class Binding {
public:
    virtual ~Binding() = default;

    virtual Kind kind() const noexcept = 0;

    // Reads `text` and stores it: a value, one more element, or for flags and counters (from environment variables) a
    // boolean or a count. Throws ValueError.
    virtual void assign(std::string_view text) = 0;

    // Flags: true, or false for a negation. Counters: one more. Values don't signal.
    virtual void signal(bool on) = 0;

    // The variable's starting value as help shows it, if it shows one.
    virtual std::optional<std::string> shown_default() const = 0;

    // The words accepted, when the value is one of fixed choices.
    virtual std::vector<std::string> choice_words() const { return {}; }

protected:
    Binding() = default;
    Binding(const Binding&) = default;
    Binding& operator=(const Binding&) = default;
};

class FlagBinding final : public Binding {
public:
    explicit FlagBinding(bool& target) noexcept : target_(target) {}
    Kind kind() const noexcept override { return Kind::flag; }
    void assign(std::string_view text) override { target_ = Reader<bool>::read(text); }
    void signal(bool on) override { target_ = on; }
    std::optional<std::string> shown_default() const override { return shown(target_); }

private:
    bool& target_;
};

class CounterBinding final : public Binding {
public:
    explicit CounterBinding(int& target) noexcept : target_(target) {}
    Kind kind() const noexcept override { return Kind::counter; }
    void assign(std::string_view text) override { target_ = Reader<int>::read(text); }
    void signal(bool) override { ++target_; }
    std::optional<std::string> shown_default() const override { return std::nullopt; }

private:
    int& target_;
};

// The side of a value binding that takes fixed choices, typed by the element.
template <class E>
class ChoiceTarget {
public:
    virtual ~ChoiceTarget() = default;
    virtual void set_choices(std::vector<std::pair<std::string, E>> choices) = 0;

protected:
    ChoiceTarget() = default;
    ChoiceTarget(const ChoiceTarget&) = default;
    ChoiceTarget& operator=(const ChoiceTarget&) = default;
};

// A value (T), an optional value (std::optional<T>) or a list (std::vector<T>).
template <class T>
class ValueBinding final : public Binding, public ChoiceTarget<element_t<T>> {
public:
    using Element = element_t<T>;

    explicit ValueBinding(T& target) noexcept : target_(target) {}

    Kind kind() const noexcept override { return Target<T>::kind; }

    void assign(std::string_view text) override {
        Element value = read(text);
        if constexpr (Target<T>::kind == Kind::list) {
            target_.push_back(std::move(value));
        } else if constexpr (Target<T>::optional) {
            target_.emplace(std::move(value));
        } else {
            target_ = std::move(value);
        }
    }

    void signal(bool) override { throw std::logic_error("cpm::args: internal error: a value was signalled"); }

    std::optional<std::string> shown_default() const override {
        if constexpr (Target<T>::kind == Kind::list) {
            std::string text;
            for (const Element& element : target_) {
                const std::optional<std::string> one = show(element);
                if (!one) return std::nullopt;
                text += (text.empty() ? "" : ", ") + *one;
            }
            if (text.empty()) return std::nullopt;
            return text;
        } else if constexpr (Target<T>::optional) {
            if (!target_) return std::nullopt;
            return show(*target_);
        } else {
            return show(target_);
        }
    }

    std::vector<std::string> choice_words() const override {
        std::vector<std::string> words;
        for (const auto& choice : choices_) words.push_back(choice.first);
        return words;
    }

    void set_choices(std::vector<std::pair<std::string, Element>> choices) override { choices_ = std::move(choices); }

private:
    Element read(std::string_view text) const {
        if (!choices_.empty()) {
            for (const auto& [word, value] : choices_) {
                if (word == text) return value;
            }
            std::string words;
            for (std::size_t i = 0; i < choices_.size(); ++i) words += (i == 0 ? "" : ", ") + choices_[i].first;
            throw ValueError("isn't one of " + words);
        }
        if constexpr (readable<Element>) {
            return Reader<Element>::read(text);
        } else {
            throw std::logic_error("cpm::args: an argument whose type has no cpm::args::Reader needs choices()");
        }
    }

    // An element as help shows it: the word of its choice, or its text.
    std::optional<std::string> show(const Element& element) const {
        if (!choices_.empty()) {
            if constexpr (std::equality_comparable<Element>) {
                for (const auto& [word, value] : choices_) {
                    if (value == element) return word;
                }
            }
            return std::nullopt;
        }
        return shown(element);
    }

    T& target_;
    std::vector<std::pair<std::string, Element>> choices_;
};

}  // namespace cpm::args::detail

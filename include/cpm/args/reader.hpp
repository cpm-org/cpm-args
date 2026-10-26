// Values: text to a C++ type (docs/spec.md §4).
#pragma once

#include <cpm/args/error.hpp>

#include <charconv>
#include <concepts>
#include <filesystem>
#include <format>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace cpm::args {

// Specialise with `static T read(std::string_view text)`, throwing ValueError (with a reason such as "isn't a size")
// when the text isn't a T.
template <class T>
struct Reader;

template <class T>
concept readable = requires(std::string_view text) {
    { Reader<T>::read(text) } -> std::convertible_to<T>;
};

namespace detail {

template <class T>
concept character = std::same_as<std::remove_cv_t<T>, char> || std::same_as<std::remove_cv_t<T>, wchar_t> ||
                    std::same_as<std::remove_cv_t<T>, char8_t> || std::same_as<std::remove_cv_t<T>, char16_t> ||
                    std::same_as<std::remove_cv_t<T>, char32_t>;

template <class T>
concept integer = std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool> && !character<T>;

enum class Number { ok, not_a_number, out_of_range };

// Decimal digits with an optional - or +.
template <integer T>
Number read_integer(std::string_view text, T& value) noexcept {
    const bool plus = text.starts_with('+');
    const std::string_view digits = plus ? text.substr(1) : text;
    const bool negative = !plus && digits.starts_with('-');
    const std::string_view magnitude = negative ? digits.substr(1) : digits;
    if (magnitude.empty() || magnitude.find_first_not_of("0123456789") != std::string_view::npos)
        return Number::not_a_number;
    if constexpr (std::is_unsigned_v<T>) {
        if (negative) {
            if (magnitude.find_first_not_of('0') != std::string_view::npos) return Number::out_of_range;
            value = 0;  // -0
            return Number::ok;
        }
    }
    const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (error == std::errc::result_out_of_range) return Number::out_of_range;
    return error == std::errc() && end == digits.data() + digits.size() ? Number::ok : Number::not_a_number;
}

// A decimal number: an optional sign, digits with an optional fraction (or a fraction alone), and an optional
// exponent; never inf or nan. Read without the C locale. Too large for a double: out_of_range; too small: zero.
Number read_double(std::string_view text, double& value) noexcept;

}  // namespace detail

template <>
struct Reader<std::string> {
    static std::string read(std::string_view text) { return std::string(text); }
};

// Arguments are UTF-8, also on Windows.
template <>
struct Reader<std::filesystem::path> {
    static std::filesystem::path read(std::string_view text);
};

template <detail::integer T>
struct Reader<T> {
    static T read(std::string_view text) {
        T value{};
        switch (detail::read_integer(text, value)) {
            case detail::Number::ok: return value;
            case detail::Number::out_of_range:
                throw ValueError(std::format("is out of range: expected an integer from {} to {}",
                                             +std::numeric_limits<T>::min(), +std::numeric_limits<T>::max()));
            case detail::Number::not_a_number: break;
        }
        throw ValueError("isn't an integer");
    }
};

template <>
struct Reader<double> {
    static double read(std::string_view text);
};

template <>
struct Reader<float> {
    static float read(std::string_view text);
};

// true, false, yes, no, on, off, 1 and 0, in any case.
template <>
struct Reader<bool> {
    static bool read(std::string_view text);
};

// Exactly one character.
template <>
struct Reader<char> {
    static char read(std::string_view text);
};

}  // namespace cpm::args

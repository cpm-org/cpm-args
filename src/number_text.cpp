// Decimal numbers without the C locale. The one place that differs by platform.

#include <cpm/args/reader.hpp>

#include <charconv>
#include <string>

#if !defined(__cpp_lib_to_chars)
#include <clocale>
#include <cmath>
#include <cstdlib>
#if defined(__APPLE__)
#include <xlocale.h>
#endif
#endif

namespace cpm::args::detail {

namespace {

bool is_digit(char c) noexcept { return c >= '0' && c <= '9'; }

// Whether `text` is [+-]? (digits (. digits*)? | . digits) ([eE] [+-]? digits)?
bool is_decimal(std::string_view text) noexcept {
    std::size_t i = 0;
    auto digits = [&] {
        const std::size_t start = i;
        while (i < text.size() && is_digit(text[i])) ++i;
        return i > start;
    };
    if (i < text.size() && (text[i] == '+' || text[i] == '-')) ++i;
    const bool whole = digits();
    bool fraction = false;
    if (i < text.size() && text[i] == '.') {
        ++i;
        fraction = digits();
    }
    if (!whole && !fraction) return false;
    if (i < text.size() && (text[i] == 'e' || text[i] == 'E')) {
        ++i;
        if (i < text.size() && (text[i] == '+' || text[i] == '-')) ++i;
        if (!digits()) return false;
    }
    return i == text.size();
}

#if defined(__cpp_lib_to_chars)

// Whether a decimal that from_chars found out of range is too large rather than too small: the decimal exponent of
// its first significant digit is above zero.
bool too_large(std::string_view text) noexcept {
    long long order = 0;  // the value is 0.d1d2... x 10^order, with d1 its first significant digit
    bool significant = false;
    bool fraction = false;
    std::size_t i = text.starts_with('-') || text.starts_with('+') ? 1 : 0;
    for (; i < text.size() && text[i] != 'e' && text[i] != 'E'; ++i) {
        if (text[i] == '.') {
            fraction = true;
        } else if (!fraction && (significant || text[i] != '0')) {
            significant = true;
            ++order;
        } else if (fraction && !significant) {
            if (text[i] == '0') --order;
            else significant = true;
        }
    }
    long long exponent = 0;
    if (i < text.size()) {
        const bool negative = text[++i] == '-';
        if (text[i] == '+' || text[i] == '-') ++i;
        for (; i < text.size() && exponent < 1'000'000'000; ++i) exponent = exponent * 10 + (text[i] - '0');
        if (negative) exponent = -exponent;
    }
    return order + exponent > 0;
}

#endif

}  // namespace

Number read_double(std::string_view text, double& value) noexcept {
    if (!is_decimal(text)) return Number::not_a_number;
    const std::string_view unsigned_or_minus = text.starts_with('+') ? text.substr(1) : text;
#if defined(__cpp_lib_to_chars)
    const auto [end, error] =
        std::from_chars(unsigned_or_minus.data(), unsigned_or_minus.data() + unsigned_or_minus.size(), value);
    if (error == std::errc()) return Number::ok;
    if (error == std::errc::result_out_of_range && !too_large(unsigned_or_minus)) {
        value = unsigned_or_minus.starts_with('-') ? -0.0 : 0.0;
        return Number::ok;
    }
    return error == std::errc::result_out_of_range ? Number::out_of_range : Number::not_a_number;
#else
    // Without floating-point from_chars (Apple's libc++): strtod in the "C" locale, so the program's locale can't change
    // what a '.' means.
    const std::string copy(unsigned_or_minus);
    char* end = nullptr;
#if defined(_WIN32)
    static const _locale_t c_locale = _create_locale(LC_ALL, "C");
    value = _strtod_l(copy.c_str(), &end, c_locale);
#else
    static const locale_t c_locale = newlocale(LC_ALL_MASK, "C", static_cast<locale_t>(nullptr));
    value = strtod_l(copy.c_str(), &end, c_locale);
#endif
    return std::isinf(value) ? Number::out_of_range : Number::ok;
#endif
}

}  // namespace cpm::args::detail

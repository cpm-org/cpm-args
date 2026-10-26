#include <cpm/args/reader.hpp>

#include <limits>

namespace cpm::args {

namespace {

// Whether `text` is `word` in any case; `word` is lowercase ASCII.
bool is(std::string_view text, std::string_view word) noexcept {
    if (text.size() != word.size()) return false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i] >= 'A' && text[i] <= 'Z' ? static_cast<char>(text[i] - 'A' + 'a') : text[i];
        if (c != word[i]) return false;
    }
    return true;
}

}  // namespace

std::filesystem::path Reader<std::filesystem::path>::read(std::string_view text) {
    std::u8string utf8;
    utf8.reserve(text.size());
    for (char c : text) utf8.push_back(static_cast<char8_t>(c));
    return std::filesystem::path(utf8);
}

double Reader<double>::read(std::string_view text) {
    double value = 0;
    switch (detail::read_double(text, value)) {
        case detail::Number::ok: return value;
        case detail::Number::out_of_range: throw ValueError("is out of range for a double");
        case detail::Number::not_a_number: break;
    }
    throw ValueError("isn't a number");
}

float Reader<float>::read(std::string_view text) {
    const double value = Reader<double>::read(text);
    if (value > std::numeric_limits<float>::max() || value < std::numeric_limits<float>::lowest())
        throw ValueError("is out of range for a float");
    return static_cast<float>(value);
}

bool Reader<bool>::read(std::string_view text) {
    for (std::string_view word : {"true", "yes", "on", "1"}) {
        if (is(text, word)) return true;
    }
    for (std::string_view word : {"false", "no", "off", "0"}) {
        if (is(text, word)) return false;
    }
    throw ValueError("isn't true or false (or yes, no, on, off, 1, 0)");
}

char Reader<char>::read(std::string_view text) {
    if (text.size() != 1) throw ValueError("isn't a single character");
    return text.front();
}

}  // namespace cpm::args

// Values (docs/spec.md §4): every built-in Reader at its limits, and a program's own.

#include <cpm/args.hpp>
#include <cpm/test.hpp>

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>

namespace args = cpm::args;

namespace {

struct Size {
    long long bytes = 0;
};

}  // namespace

template <>
struct cpm::args::Reader<Size> {
    static Size read(std::string_view text) {
        if (text.ends_with("KB")) return {Reader<long long>::read(text.substr(0, text.size() - 2)) * 1000};
        throw ValueError("isn't a size such as 10KB");
    }
};

TEST_CASE("strings and paths take anything") {
    CHECK(args::Reader<std::string>::read("") == "");
    CHECK(args::Reader<std::string>::read("-x y") == "-x y");
    CHECK(args::Reader<std::filesystem::path>::read("dir/file.txt") == std::filesystem::path("dir/file.txt"));
    CHECK(args::Reader<std::filesystem::path>::read("caf\xC3\xA9").u8string() == u8"café");  // UTF-8
}

TEST_CASE("integers are decimal and in range") {
    CHECK(args::Reader<int>::read("42") == 42);
    CHECK(args::Reader<int>::read("-42") == -42);
    CHECK(args::Reader<int>::read("+7") == 7);
    CHECK(args::Reader<std::uint16_t>::read("65535") == 65535);
    CHECK(args::Reader<std::int64_t>::read("-9223372036854775808") == std::numeric_limits<std::int64_t>::min());
    CHECK(args::Reader<unsigned>::read("-0") == 0u);
    CHECK_THROWS_WITH(args::Reader<std::uint16_t>::read("65536"), "is out of range: expected an integer from 0 to 65535");
    CHECK_THROWS_WITH(args::Reader<unsigned>::read("-1"), "is out of range: expected an integer from 0 to 4294967295");
    CHECK_THROWS_WITH(args::Reader<std::int8_t>::read("-129"), "is out of range: expected an integer from -128 to 127");
    for (const char* bad : {"", "abc", "1.5", "1e3", "0x10", " 1", "1 ", "+", "-", "+-1", "--1", "1_000"}) {
        INFO("\"{}\"", bad);
        CHECK_THROWS_WITH(args::Reader<int>::read(bad), "isn't an integer");
    }
}

TEST_CASE("floating-point numbers") {
    CHECK(args::Reader<double>::read("2.5") == 2.5);
    CHECK(args::Reader<double>::read("-.5") == -0.5);
    CHECK(args::Reader<double>::read("5.") == 5.0);
    CHECK(args::Reader<double>::read("+1e3") == 1000.0);
    CHECK(args::Reader<double>::read("7") == 7.0);
    CHECK(args::Reader<double>::read("1e-400") == 0.0);  // too small: zero
    CHECK(args::Reader<float>::read("0.25") == 0.25f);
    CHECK_THROWS_WITH(args::Reader<double>::read("1e400"), "is out of range for a double");
    CHECK_THROWS_WITH(args::Reader<float>::read("1e300"), "is out of range for a float");
    for (const char* bad : {"", ".", "e5", "1e", "inf", "nan", "0x1p3", "1,5", "--1"}) {
        INFO("\"{}\"", bad);
        CHECK_THROWS_WITH(args::Reader<double>::read(bad), "isn't a number");
    }
}

TEST_CASE("booleans in words") {
    for (const char* yes : {"true", "TRUE", "yes", "On", "1"}) CHECK(args::Reader<bool>::read(yes));
    for (const char* no : {"false", "No", "OFF", "0"}) CHECK(!args::Reader<bool>::read(no));
    CHECK_THROWS_WITH(args::Reader<bool>::read("maybe"), "isn't true or false (or yes, no, on, off, 1, 0)");
}

TEST_CASE("characters are exactly one") {
    CHECK(args::Reader<char>::read(",") == ',');
    CHECK_THROWS_WITH(args::Reader<char>::read(""), "isn't a single character");
    CHECK_THROWS_WITH(args::Reader<char>::read("ab"), "isn't a single character");
}

TEST_CASE("a program's own Reader") {
    CHECK(args::Reader<Size>::read("10KB").bytes == 10000);
    CHECK_THROWS_WITH(args::Reader<Size>::read("10"), "isn't a size such as 10KB");
    static_assert(args::readable<Size>);
    static_assert(args::readable<std::filesystem::path>);
    static_assert(!args::readable<std::string_view>);  // it would dangle
}

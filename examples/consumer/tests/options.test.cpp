#include <consumer/options.hpp>

#include <cpm/test.hpp>

#include <sstream>

namespace {

struct Run {
    Options options;
    std::ostringstream out;
    std::ostringstream err;
    std::optional<int> exit_code;

    template <std::size_t N>
    explicit Run(const char* (&argv)[N]) : exit_code(read_options(static_cast<int>(N), argv, options, out, err)) {}
};

}  // namespace

TEST_CASE("todo add reads its words and options") {
    const char* argv[] = {"todo", "add", "buy", "milk", "--due", "friday", "-d", "home.txt"};
    const Run run(argv);
    CHECK(!run.exit_code.has_value());
    CHECK(run.options.command == "add");
    CHECK(run.options.words == (std::vector<std::string>{"buy", "milk"}));
    CHECK(run.options.due == std::optional<std::string>("friday"));
    CHECK(run.options.data == "home.txt");
}

TEST_CASE("todo list, and its help") {
    const char* list[] = {"todo", "list", "-a"};
    const Run listed(list);
    CHECK(listed.options.command == "list");
    CHECK(listed.options.all);

    const char* help[] = {"todo", "list", "--help"};
    const Run helped(help);
    CHECK(helped.exit_code == std::optional<int>(0));
    CHECK(helped.out.str().starts_with("Shows the items\n\nUsage: todo list [OPTIONS]\n"));
}

TEST_CASE("todo with a mistake") {
    const char* argv[] = {"todo", "ad", "x"};
    const Run run(argv);
    CHECK(run.exit_code == std::optional<int>(2));
    CHECK(run.err.str().starts_with("error: unknown command \"ad\" (did you mean add?)\n"));
}

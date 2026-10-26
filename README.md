# cpm-args

Command-line arguments for C++20, made for [CPM](https://github.com/meirborowski/CPM). Bind options and positional arguments to your own variables, nest commands like `git commit`, and get generated help and clear errors.

```sh
cpm install cpm-args
```

```cpp
#include <cpm/args.hpp>

#include <filesystem>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    cpm::args::Parser parser("serve", "Serves a directory over HTTP");
    parser.version("1.4.0");

    int port = 8080;  // a variable's starting value is its default
    bool verbose = false;
    std::vector<std::string> headers;
    std::filesystem::path root = ".";

    parser.option(port, "--port", "-p").help("port to listen on").env("SERVE_PORT");
    parser.flag(verbose, "--verbose", "-v").help("print every request");
    parser.option(headers, "--header", "-H").help("an extra response header");
    parser.positional(root, "ROOT").help("the directory to serve").optional();

    // Prints help, the version or an error, and returns the exit code, when that's what was asked for.
    if (const auto exit_code = parser.parse_main(argc, argv)) return *exit_code;

    // ... serve root on port
}
```

```
$ serve --help
Serves a directory over HTTP

Usage: serve [OPTIONS] [ROOT]

Arguments:
  [ROOT]  the directory to serve (default: .)

Options:
  -p, --port <PORT>      port to listen on (default: 8080) [env: SERVE_PORT]
  -v, --verbose          print every request
  -H, --header <HEADER>  an extra response header (repeatable)
  -h, --help             show this help
  -V, --version          show the version

$ serve --prot 80
error: unknown option --prot (did you mean --port?)

Usage: serve [OPTIONS] [ROOT]

For more information, try 'serve --help'.
```

The [specification](docs/spec.md) has every rule in detail.

## Arguments

| Function | For | Given as |
|---|---|---|
| `flag(bool&, names...)` | a switch | `--verbose` |
| `counter(int&, names...)` | a count | `-vvv` |
| `option(T&, names...)` | a value | `--port 80`, `--port=80`, `-p 80`, `-p80` |
| `option(std::vector<T>&, names...)` | values | `-H a -H b` |
| `option(std::optional<T>&, names...)` | a value, or none | `--limit 5` |
| `positional(T&, "NAME")` | a positional argument | `ROOT` |
| `positional(std::vector<T>&, "NAME")` | the rest | `FILES...` |

Names are `--long` and `-s`, and every option needs a long one. An option given twice keeps its last value.

Settings chain after the definition:

| Setting | |
|---|---|
| `.help(text)` | the description in help |
| `.value_name("PORT")` | the placeholder in help |
| `.required()` | must be given |
| `.optional()` | for positional arguments, which are otherwise required |
| `.env("NAME")` | when not given, take it from this environment variable |
| `.choices({{"fast", Mode::fast}, {"small", Mode::small}})` | accept only these words |
| `.negatable()` | for flags: `--no-color` sets `false` |
| `.hidden()` | leave it out of help |
| `.default_text("…")` | what help shows as the default |

Mistakes in these definitions, such as a name used twice or a setting that doesn't fit, throw `std::logic_error` at once, because they are bugs in the program rather than in its command line.

## Values

Strings, `std::filesystem::path`, integers (checked against their type's range), `float` and `double`, `bool` (`yes`, `no`, `on`, `off`, …) and `char` are built in. Read your own types by specialising `cpm::args::Reader`:

```cpp
template <>
struct cpm::args::Reader<Size> {
    static Size read(std::string_view text);  // throws cpm::args::ValueError("isn't a size such as 10MB")
};
```

## The command line

- `-abc` is `-a -b -c`. When one of them takes a value, the rest of the group is the value, so `-vp8080` is `-v -p 8080`.
- An option's value is the next argument even when it starts with `-`, so `--offset -5` works.
- Negative numbers such as `-5` are positional arguments, unless the program defines digits as short options.
- After `--`, every argument is positional.
- `-` alone is positional.
- Options and positional arguments can come in any order.

## Commands

```cpp
cpm::args::Parser parser("cpm", "A package manager");
bool verbose = false;
parser.flag(verbose, "--verbose", "-v");

cpm::args::Command& build = parser.command("build", "Compiles the project");
bool release = false;
build.flag(release, "--release");

if (const auto exit_code = parser.parse_main(argc, argv)) return *exit_code;
if (parser.outcome().command == &build) ...
```

- Commands nest to any depth.
- A command with subcommands needs one of them, unless `.subcommand_optional()` is set.
- A parent's options also work under its subcommands, before or after the subcommand's name: `cpm build -v` and `cpm -v build` both work.
- `parser.outcome()` says which command was selected.

## Parsing without `parse_main`

`parser.parse(argc, argv)` returns an `Outcome`: its `action` is `run`, `help` or `version`, and its `command` is the command selected. It throws `cpm::args::UsageError` when the command line doesn't fit. `help_text()` and `usage_text()` give the text to print. To parse a list of arguments with an environment of your own, as tests do, pass a `std::span<const std::string_view>` and a `cpm::args::Environment`.

Help wraps at 80 columns; `parser.width(n)` changes that. `-h`, `--help`, `-V` and `--version` are added for you, unless your program uses those names itself.

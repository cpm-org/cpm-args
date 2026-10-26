# cpm-args 0.1.0: specification

cpm-args reads command lines in C++20. Options, flags and positional arguments are bound to variables of the program's own types. Commands nest the way `git commit` and `cpm build` do. Help is generated, and mistakes get a clear message with a suggestion. It is a [cpm-org](https://github.com/cpm-org) library: original code with its own API, built and tested with [CPM](https://github.com/meirborowski/CPM). This document is what the code is written against. The README will be the user's guide.

## 1. Scope

0.1.0 covers:

- **Arguments:** flags, counters (`-vvv`), options with values, repeatable options, and positional arguments, each bound to a variable.
- **Values:** strings, paths, integers with range checks, floating-point numbers, booleans, `std::optional` and `std::vector` of those, a fixed set of choices, and the program's own types through a customization point.
- **Syntax:** `--name value`, `--name=value`, `-n value`, `-nvalue`, combined short flags (`-abc`), `--`, and negative numbers as values.
- **Commands:** subcommands, nested to any depth. Options are inherited by subcommands.
- **Environment variables** as fallbacks for options.
- **Help and version:** generated help with usage, sections and wrapping, `-h`/`--help`, and `-V`/`--version`.
- **Errors** that say what is wrong and suggest the name that was probably meant. Mistakes in how a program defines its arguments are caught when they are defined.

Not in 0.1.0:

- Abbreviated long options (`--verb` for `--verbose`).
- Response files (`@args.txt`) and configuration files.
- Shell completion scripts.
- Groups of options that exclude or require each other, and value validation beyond types, ranges and choices. Programs check those after parsing.
- Prompting for missing values.
- Translations.
- UTF-16 command lines on Windows: arguments are taken as given to `main`, which programs should treat as UTF-8.

## 2. The package

| | |
|---|---|
| Repository | `github.com/cpm-org/cpm-args`, MIT |
| Registry name | `cpm-args` |
| Header | `<cpm/args.hpp>`, which includes the parts in `<cpm/args/*.hpp>` |
| Namespace | `cpm::args` |
| Standard | `"std": "c++20"` |
| Type | `static-lib` |
| Tests | `tests/*.test.cpp` on cpm-test, run with `cpm test` |
| CI | Linux (GCC and Clang), macOS (Apple Clang) and Windows (MinGW GCC), with CPM installed by its installer: strict warnings as errors, release, and AddressSanitizer with UBSan on Linux |

It depends on nothing but the standard library.

## 3. Defining arguments

```cpp
cpm::args::Parser parser("serve", "Serves a directory over HTTP");
parser.version("1.4.0");

int port = 8080;
bool verbose = false;
std::vector<std::string> headers;
std::filesystem::path root = ".";

parser.option(port, "--port", "-p").help("port to listen on").value_name("PORT").env("SERVE_PORT");
parser.flag(verbose, "--verbose", "-v").help("print every request");
parser.option(headers, "--header", "-H").help("an extra response header");
parser.positional(root, "ROOT").help("the directory to serve").optional();

if (const auto exit_code = parser.parse_main(argc, argv)) return *exit_code;
```

### 3.1 Names

- A long name is `--` followed by lowercase letters, digits and `-`, starting with a letter: `--port`, `--dry-run`.
- A short name is `-` and one letter or digit: `-p`, `-2`.
- An argument has one or more names, at least one of them long. Its first long name is the one help and errors use.
- Positional arguments have a display name instead, conventionally uppercase: `ROOT`, `FILE`.

### 3.2 Kinds

| Function | Binds | Given as | Result |
|---|---|---|---|
| `flag(bool&, names...)` | a switch | `--verbose` | `true`. It takes no value. |
| `counter(int&, names...)` | a count | `-v -v`, `-vv` | one more for each time it is given |
| `option(T&, names...)` | a value | `--port 80` | the value, read as `T` (§4). Given twice, the last one wins. |
| `option(std::vector<T>&, names...)` | values | `-H a -H b` | every value, in order, after any the vector already held |
| `option(std::optional<T>&, names...)` | a value, or none | `--limit 5` | set when given, left as is otherwise |
| `positional(T&, display_name)` | a positional argument | `ROOT` | the value |
| `positional(std::vector<T>&, display_name)` | the rest | `FILES...` | every remaining positional argument |

- A variable keeps its value when its argument isn't given, so the value it starts with is the default.
- Variables are written while parsing, so they must outlive `parse`.

### 3.3 Settings

Each definition returns an `Argument&` for chaining:

| Setting | Effect |
|---|---|
| `.help(text)` | the description in help |
| `.value_name(name)` | the placeholder in help, `--port <PORT>`. The default is the long name in uppercase. |
| `.required()` | it must be given (or come from its environment variable) |
| `.optional()` | for positional arguments, which are required unless this is set. Optional ones must come after the required ones. |
| `.env(name)` | when not given, take the value from this environment variable, if it is set and not empty (§6) |
| `.choices({{"fast", Mode::fast}, {"small", Mode::small}})` | accept only these words, storing the value each one maps to. Help lists the words. |
| `.negatable()` | for flags: also define `--no-<name>`, which sets `false` |
| `.hidden()` | leave it out of help |
| `.default_text(text)` | show this as the default in help. Without it, help shows the variable's starting value when that value can be formatted with `std::format`, isn't empty, and isn't `false`. |

### 3.4 Definition errors

A program that defines its arguments wrongly has a bug, not a user error. These throw `std::logic_error` as soon as they are defined, except the order of required and optional positional arguments, which `parse` checks when it starts, since `.optional()` may come later:
- a name that isn't valid, or no long name;
- a name already used by the command or one of its ancestors;
- a required positional after an optional one, or anything after a vector positional;
- a positional argument in a command that has subcommands;
- choices with a repeated word, `negatable()` on anything but a flag, or `optional()` on an option.

## 4. Values

`Reader<T>` turns text into a `T`:

```cpp
template <class T>
struct Reader;  // static T read(std::string_view text); throws ValueError when the text isn't a T
```

| Type | Accepts |
|---|---|
| `std::string` | anything |
| `std::filesystem::path` | anything |
| integer types | decimal digits with an optional `-` (or `+`), within the type's range |
| `float`, `double` | decimal numbers, including exponents, within the type's range, read without the C locale |
| `bool` | `true`, `false`, `yes`, `no`, `on`, `off`, `1` and `0`, in any case |
| `char` | exactly one character |

`ValueError` says what was expected: `expected an integer from 0 to 65535`. The parser adds which argument and value it was, and that becomes the `UsageError` (§7): `--port: "70000" is out of range: expected an integer from 0 to 65535`.

A program reads its own types by specialising `Reader`:

```cpp
template <>
struct cpm::args::Reader<Size> {
    static Size read(std::string_view text);  // "10MB" -> Size{10'000'000}
};
```

Help shows `.value_name`. Without one, it shows the argument's name in uppercase.

## 5. The command line

`parse` reads the arguments after the program name, from left to right:

| Argument | Means |
|---|---|
| `--name` | a flag, counter or negation, or an option whose value is the next argument |
| `--name=value` | an option with its value. `=` on a flag is an error. |
| `-n` | as `--name`, by short name |
| `-nvalue` | an option with its value attached |
| `-abc` | the flags `-a`, `-b` and `-c`. When one of them takes a value, the rest of the group is that value: `-vp8080` is `-v -p 8080`. |
| `--` | everything after it is positional |
| `-` | a positional argument (by convention, standard input) |
| `-5`, `-0.5` | a positional argument, when it reads as a number and the command has no short option named by a digit |
| anything else | the next positional argument, or the subcommand's name |

- An option's value is always the next argument, even when it starts with `-`: `--offset -5`.
- Options and positional arguments can be mixed in any order.

### 5.1 Commands

```cpp
auto& build = parser.command("build", "Compiles the project");
bool release = false;
build.flag(release, "--release");

const cpm::args::Outcome outcome = parser.parse(argc, argv);
if (outcome.command == &build) ...
```

- `command(name, about)` adds a subcommand and returns a `Command&`, which defines arguments, and further commands, the same way `Parser` does.
- The first argument that isn't an option, and is a command's name, selects that command. The arguments after it belong to it.
- A subcommand also accepts its ancestors' options, before or after its name, so `--verbose` works anywhere.
- A command with subcommands needs one of them, unless `.subcommand_optional()` is set.
- `Outcome::command` points to the deepest command selected, and `Outcome::path` names the commands, such as `{"build"}`.

## 6. Environment variables

- An option with `.env(name)` that isn't given on the command line takes its value from that variable when the variable is set and not empty. The value is read as if it had been given.
- A flag reads the variable as a `bool` (§4).
- Errors in such values name the variable: `SERVE_PORT: "eighty" isn't an integer`.
- `parse` reads variables through a function, `std::getenv` by default, so tests can supply their own.

## 7. Parsing and errors

```cpp
struct Outcome {
    enum class Action { run, help, version };
    Action action;                  // run: the program goes on; help or version was asked for
    const Command* command;         // the deepest command selected
    std::vector<std::string> path;  // the names of the commands selected, outermost first
};

Outcome parse(int argc, const char* const* argv);                 // skips argv[0]
Outcome parse(std::span<const std::string_view> args, Environment environment = system_environment);
std::optional<int> parse_main(int argc, const char* const* argv, std::ostream& out = std::cout,
                              std::ostream& err = std::cerr);
```

- `parse` throws `UsageError` for a command line that doesn't fit. Its `what()` is the message, and `command()` is the command whose usage applies.
- Help is asked for with `-h` or `--help` on any command, and the version with `-V` or `--version` on the root, when `version()` was given. They win over everything else, including errors, so `serve --help --bad` shows help. They aren't defined when the program uses those names itself.
- `parse_main` is what a `main` calls:
  - for help, it writes the selected command's help to `out` and returns 0;
  - for the version, it writes `<name> <version>` to `out` and returns 0;
  - for a usage error, it writes the error, the usage line and a hint to `err`, and returns 2;
  - otherwise it returns `nullopt`, and the program goes on.
- `outcome()` returns the `Outcome` of the last parse that didn't throw, so a program that uses `parse_main` can still tell which command was selected.

**Messages:**

| Problem | Message |
|---|---|
| unknown option | `unknown option --prot (did you mean --port?)` |
| unknown command | `unknown command "biuld" (did you mean build?)` |
| missing value | `--port needs a value` |
| bad value | `--port: "abc" isn't an integer` |
| value on a flag | `--verbose doesn't take a value` |
| missing argument | `--root is required`, or `missing ROOT` |
| no subcommand | `missing a command: build, test or run` |
| extra argument | `unexpected argument "extra"` |
| a choice that isn't one | `--mode: "quick" isn't one of fast, small` |

Suggestions come from the command's names (and its ancestors' option names) within an edit distance of 2. The closest one is offered, and none when there is a tie.

As `parse_main` writes it:

```
error: unknown option --prot (did you mean --port?)

Usage: serve [OPTIONS] [ROOT]

For more information, try 'serve --help'.
```

## 8. Help

```
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
```

- **Usage** lists `[OPTIONS]` when there are any, positional arguments as `NAME`, `[NAME]` or `NAME...`, and `<COMMAND>` (or `[COMMAND]` when one is optional). For a subcommand it starts with the full path: `cpm build [OPTIONS]`.
- **Sections** come in order: Commands, Arguments, Options. Each is aligned in two columns.
- Help text **wraps** at `width` columns (default 80, set with `Parser::width`), with continuation lines indented to the second column. A first column too wide to leave 30 columns puts the help on the next line instead.
- **Notes** after the help text: `(default: …)`, `(repeatable)`, `[env: NAME]`, and the choices: `[fast, small]`.
- `help_text(command)` and `usage_text(command)` return the text, for programs that print it themselves.

## 9. Structure

Each part has one job and sits behind a small interface. Nothing does I/O except `parse_main`'s writing to the given streams and the default environment lookup.

| Part | Job |
|---|---|
| `names` | checks long, short and command names |
| `reader` | `Reader<T>` and `ValueError`, in the header |
| `number_text` | integers and floating-point numbers from text, without the C locale (where floating-point `from_chars` is missing, `strtod_l` in the `"C"` locale, as cpm-json does) |
| `binding` | where a value goes: the type-erased link from an argument to its variable, one implementation per kind (§3.2) |
| `argument` | `Argument`: names, settings and binding |
| `command` | `Command`: arguments, subcommands, and name lookup through the ancestors |
| `tokens` | classifies each command-line argument: long option, short group, `--`, or plain |
| `matcher` | walks the tokens against the commands, fills in the variables, then applies environment variables and checks what is required |
| `suggest` | edit distance and the closest name |
| `help` | usage and help text, with wrapping |
| `environment` | `Environment`, and the `std::getenv` default |
| `parser` | `Parser`: the root command, the version, `parse` and `parse_main` |

## 10. Testing

- **Tokens:** every form in §5.
- **Values:** every built-in type at its limits, and a user `Reader`.
- **Kinds and settings:** each one in §3.2 and §3.3, including starting values kept as defaults.
- **Commands:** nesting, inherited options before and after a command's name, and optional subcommands.
- **Environment:** an injected environment, where the command line wins over it, and errors that name the variable.
- **Errors:** every message in §7, with and without suggestions.
- **Help:** whole help texts compared exactly, including wrapping at narrow widths, and hidden arguments.
- **`parse_main`:** exit codes, and what goes to each stream.
- **Definition errors:** every rule in §3.4.
- **Example project:** `examples/consumer` uses cpm-args through a path dependency, as users do.

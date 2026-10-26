# Changelog

## 0.1.0

The first release.

- **Arguments bound to typed variables:** flags, counters, options with values, repeatable options (`std::vector`), optional values (`std::optional`), and positional arguments, the last of which can take the rest.
- **Values:** strings, paths, range-checked integers, floating-point numbers read without the C locale, booleans in words, characters, and fixed choices. A program's own types work through `Reader<T>`.
- **Command lines:** `--name value`, `--name=value`, `-n value`, `-nvalue`, combined short flags, `--`, and negative numbers as values.
- **Subcommands,** nested to any depth, which accept their parents' options.
- **Environment variables** as fallbacks.
- **Generated help** with defaults, choices and environment variables, wrapped to a width, plus `-h`/`--help` and `-V`/`--version`.
- **Errors with suggestions:** `unknown option --prot (did you mean --port?)`. `parse_main` handles help, the version and errors for `main()`.

#include "help.hpp"

#include <algorithm>
#include <optional>

namespace cpm::args::detail {

namespace {

// Help rows are indented by this much, and two spaces separate the columns.
constexpr std::size_t indent = 2;
constexpr std::size_t gap = 2;
// A first column that would leave less than this for the help puts the help on the next line.
constexpr std::size_t min_help_width = 30;

struct Row {
    std::string left;
    std::string right;
};

std::string positional_label(const Argument& positional) {
    std::string label = positional.display_name();
    if (positional.kind() == Kind::list) label += "...";
    return positional.is_optional() ? "[" + label + "]" : label;
}

std::string option_label(const Argument& option) {
    std::string label;
    for (char name : option.short_names()) label += std::string("-") + name + ", ";
    if (label.empty()) label = "    ";  // so long names line up with those after a short one
    for (std::size_t i = 0; i < option.long_names().size(); ++i) {
        if (i > 0) label += ", ";
        label += (option.is_negatable() ? "--[no-]" : "--") + option.long_names()[i];
    }
    if (option.kind() == Kind::value || option.kind() == Kind::list) label += " <" + option.value_name() + ">";
    return label;
}

// The help text with its notes: (default: …), (repeatable), [choices], [env: NAME].
std::string described(const Argument& argument) {
    std::string text = argument.help();
    auto note = [&](const std::string& part) { text += (text.empty() ? "" : " ") + part; };
    if (const std::optional<std::string> shown = argument.shown_default()) note("(default: " + *shown + ")");
    if (!argument.is_positional() && (argument.kind() == Kind::list || argument.kind() == Kind::counter))
        note("(repeatable)");
    const std::vector<std::string> words = argument.choice_words();
    if (!words.empty()) {
        std::string list;
        for (const std::string& word : words) list += (list.empty() ? "" : ", ") + word;
        note("[" + list + "]");
    }
    if (!argument.env().empty()) note("[env: " + argument.env() + "]");
    return text;
}

std::vector<Row> option_rows(const Command& command) {
    std::vector<Row> rows;
    for (const auto& option : command.options()) {
        if (!option->is_hidden()) rows.push_back({option_label(*option), described(*option)});
    }
    return rows;
}

// -h/--help and -V/--version, as far as the program left those names free.
Row builtin_row(const Command& command, char short_name, std::string_view long_name, std::string_view help) {
    const std::string prefix = command.find_short(short_name) == nullptr ? std::string("-") + short_name + ", " : "    ";
    return {prefix + "--" + std::string(long_name), std::string(help)};
}

void section(std::string& out, std::string_view title, const std::vector<Row>& rows, std::size_t width) {
    if (rows.empty()) return;
    std::size_t column = 0;
    for (const Row& row : rows) {
        if (indent + row.left.size() + gap + min_help_width <= width) column = std::max(column, row.left.size());
    }
    const std::size_t help_start = indent + column + gap;
    const std::size_t help_width = width > help_start + 10 ? width - help_start : 10;
    out += "\n";
    out += title;
    out += ":\n";
    for (const Row& row : rows) {
        const std::vector<std::string> lines = wrap(row.right, help_width);
        out += std::string(indent, ' ') + row.left;
        std::size_t first = 0;
        if (row.left.size() > column) {
            out += '\n';  // too wide: the help starts on the next line
        } else if (!lines.empty()) {
            out += std::string(help_start - indent - row.left.size(), ' ') + lines[0] + '\n';
            first = 1;
        } else {
            out += '\n';
        }
        for (std::size_t i = first; i < lines.size(); ++i) out += std::string(help_start, ' ') + lines[i] + '\n';
    }
}

}  // namespace

std::vector<std::string> wrap(std::string_view text, std::size_t width) {
    std::vector<std::string> lines;
    std::string line;
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && (text[i] == ' ' || text[i] == '\n')) ++i;
        const std::size_t start = i;
        while (i < text.size() && text[i] != ' ' && text[i] != '\n') ++i;
        if (start == i) break;
        const std::string_view word = text.substr(start, i - start);
        if (!line.empty() && line.size() + 1 + word.size() > width) {
            lines.push_back(std::move(line));
            line.clear();
        }
        if (!line.empty()) line += ' ';
        line += word;
    }
    if (!line.empty()) lines.push_back(std::move(line));
    return lines;
}

std::string usage_line(const Command& command) {
    std::string line = "Usage: " + command.path() + " [OPTIONS]";
    for (const auto& positional : command.positionals()) line += " " + positional_label(*positional);
    if (!command.commands().empty()) line += command.needs_subcommand() ? " <COMMAND>" : " [COMMAND]";
    return line;
}

std::string help_text(const Command& command, std::size_t width, bool has_version) {
    std::string out;
    if (!command.about().empty()) out += command.about() + "\n\n";
    out += usage_line(command) + "\n";

    std::vector<Row> commands;
    for (const auto& sub : command.commands()) commands.push_back({sub->name(), sub->about()});
    section(out, "Commands", commands, width);

    std::vector<Row> positionals;
    for (const auto& positional : command.positionals()) {
        if (!positional->is_hidden()) positionals.push_back({positional_label(*positional), described(*positional)});
    }
    section(out, "Arguments", positionals, width);

    const bool is_root = command.parent() == nullptr;
    const bool version_free = has_version && command.find_long("version") == nullptr;
    std::vector<Row> options = option_rows(command);
    if (command.find_long("help") == nullptr) options.push_back(builtin_row(command, 'h', "help", "show this help"));
    if (is_root && version_free) options.push_back(builtin_row(command, 'V', "version", "show the version"));
    section(out, "Options", options, width);

    if (!is_root) {
        std::vector<Row> inherited;
        for (const Command* c = command.parent(); c != nullptr; c = c->parent()) {
            const std::vector<Row> rows = option_rows(*c);
            inherited.insert(inherited.end(), rows.begin(), rows.end());
        }
        if (version_free) inherited.push_back(builtin_row(command, 'V', "version", "show the version"));
        section(out, "Inherited options", inherited, width);
    }
    return out;
}

}  // namespace cpm::args::detail

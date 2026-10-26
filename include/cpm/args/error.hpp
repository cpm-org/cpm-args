// The exceptions for bad command lines (docs/spec.md §4, §7). Mistakes in defining arguments throw std::logic_error.
#pragma once

#include <stdexcept>
#include <string>

namespace cpm::args {

class Command;

// The base of the exceptions cpm-args throws for bad input.
class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Text that isn't a value of the type asked for. what() is the reason, said of the value: "isn't an integer", "is out
// of range: expected an integer from 0 to 65535". The parser adds which argument and value it was.
class ValueError : public Error {
public:
    using Error::Error;
};

// A command line that doesn't fit the arguments defined: "unknown option --prot (did you mean --port?)".
class UsageError : public Error {
public:
    UsageError(const std::string& message, const Command& command) : Error(message), command_(&command) {}

    // The command whose usage applies.
    const Command& command() const noexcept { return *command_; }

private:
    const Command* command_;
};

}  // namespace cpm::args

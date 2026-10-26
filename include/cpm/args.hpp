// cpm-args: command-line arguments for C++20. See README.md for a guide and docs/spec.md for the specification.
//
//   #include <cpm/args.hpp>
//
//   int main(int argc, char** argv) {
//       cpm::args::Parser parser("serve", "Serves a directory over HTTP");
//       int port = 8080;                                          // the starting value is the default
//       parser.option(port, "--port", "-p").help("port to listen on");
//       if (const auto exit_code = parser.parse_main(argc, argv)) return *exit_code;  // help, version, errors
//       ...
//   }
#pragma once

#include <cpm/args/argument.hpp>
#include <cpm/args/command.hpp>
#include <cpm/args/environment.hpp>
#include <cpm/args/error.hpp>
#include <cpm/args/parser.hpp>
#include <cpm/args/reader.hpp>

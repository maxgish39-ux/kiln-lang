/* Kiln native runtime: entry point.
 *
 * The implementation is split into sections under src/. They are included here
 * in dependency order, so this file is still a single C translation unit:
 *
 *   src/lexer.inc        tokens and the lexer
 *   src/values.inc       value, list, dictionary, function and class types
 *   src/parser.inc       error raising and the parser
 *   src/environment.inc  environments, lists, dictionaries, class instances
 *   src/calls.inc        function and class calls
 *   src/graphics.inc     optional raylib key and colour helpers
 *   src/helpers.inc      equality, sorting, string building, JSON, time, paths
 *   src/network.inc      HTTP and HTTPS client (https uses the system curl)
 *   src/regex.inc        regular expressions (POSIX extended syntax)
 *   src/modules.inc      built-in module functions
 *   src/interpreter.inc  module loading and tree-walking evaluation
 *   src/vm.inc           bytecode compiler and virtual machine (default)
 *   src/main.inc         program entry point
 *
 * Build: cc -std=c11 -O2 runtime/kiln.c -lm -o kiln
 */
#include "../src/lexer.inc"
#include "../src/values.inc"
#include "../src/parser.inc"
#include "../src/environment.inc"
#include "../src/calls.inc"
#include "../src/graphics.inc"
#include "../src/helpers.inc"
#include "../src/network.inc"
#include "../src/regex.inc"
#include "../src/modules.inc"
#include "../src/interpreter.inc"
#include "../src/vm.inc"
#include "../src/main.inc"

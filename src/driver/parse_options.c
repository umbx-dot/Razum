#include "driver/driver.h"

#include <stdio.h>
#include <string.h>

static const char USAGE[] =
    "usage: rzc <file.rz> [-o <file.exe>] [-S] [--asm] [--run] [--check] [--stdin] [--dll]\n"
    "  -o <path>  executable to write (default: the source name with .exe)\n"
    "  -S         write only the .asm file next to the executable path, do not assemble\n"
    "  --asm      build the executable and keep the .asm file next to it\n"
    "  --run      run the program after building and exit with its exit code\n"
    "  --check    only report errors; write nothing\n"
    "  --dll      build a DLL from the functions marked export; also writes <name>.def\n"
    "             and <name>.h for C and C++ (default output: the source name with .dll)\n"
    "  --stdin    read the source text from standard input; <file.rz> still names it\n"
    "             for error messages and for finding imported modules\n"
    "  -O         optimize: keep local variables in registers (see docs/optimizer.md)\n"
    "  -march=znver3  optimize and allow Zen 3 instructions in generated code\n"
    "  -I <folder>  also look for imported modules in this folder (after the importing file's\n"
    "             folder and the main file's folder, before the built-in libraries)\n"
    "  --gui      make a window program (no console window); the same as program gui; in the source\n"
    "  --console  make a console program even if the source says program gui;\n"
    "  --symbols  print every declaration (for editors): kind, name, detail, file, line,\n"
    "             column, scope start and end line, separated by tabs; errors go to stderr\n"
    "  --source <path>  print a built-in library file, such as <rzc>/libraries/stdio/output/print_long.rz\n";

static bool usage_error(const char *message, const char *detail) {
    fprintf(stderr, "rzc: %s%s\n%s", message, detail, USAGE);
    return false;
}

bool parse_options(int argc, char **argv, Options *options) {
    *options = (Options){0};
    for (int index = 1; index < argc; index++) {
        const char *argument = argv[index];
        if (strcmp(argument, "-o") == 0) {
            if (index + 1 >= argc) {
                return usage_error("-o needs a path", "");
            }
            options->output_path = argv[++index];
        } else if (strcmp(argument, "-I") == 0) {
            if (index + 1 >= argc) {
                return usage_error("-I needs a folder", "");
            }
            list_push(&import_folders, argv[++index]);
        } else if (strcmp(argument, "-O") == 0) {
            options->optimize = true;
        } else if (strcmp(argument, "-march=znver3") == 0) {
            options->optimize = true;
            options->zen3 = true;
        } else if (strcmp(argument, "-S") == 0) {
            options->asm_only = true;
        } else if (strcmp(argument, "--asm") == 0) {
            options->keep_asm = true;
        } else if (strcmp(argument, "--check") == 0) {
            options->check_only = true;
        } else if (strcmp(argument, "--stdin") == 0) {
            options->read_stdin = true;
        } else if (strcmp(argument, "--dll") == 0) {
            options->dll = true;
        } else if (strcmp(argument, "--run") == 0) {
            options->run = true;
        } else if (strcmp(argument, "--gui") == 0) {
            options->window_program = 1;
        } else if (strcmp(argument, "--console") == 0) {
            options->window_program = -1;
        } else if (strcmp(argument, "--symbols") == 0) {
            options->symbols = true;
        } else if (strcmp(argument, "--source") == 0) {
            if (index + 1 >= argc) {
                return usage_error("--source needs a library path", "");
            }
            options->show_source = argv[++index];
        } else if (strcmp(argument, "-h") == 0 || strcmp(argument, "--help") == 0) {
            fputs(USAGE, stdout);
            options->help = true;
            return true;
        } else if (argument[0] == '-') {
            return usage_error("unknown option ", argument);
        } else if (options->source_path != NULL) {
            return usage_error("only one source file is allowed, got a second one: ", argument);
        } else {
            options->source_path = argument;
        }
    }
    if (options->source_path == NULL && options->show_source == NULL) {
        return usage_error("no source file given", "");
    }
    if (options->dll && options->run) {
        return usage_error("--run cannot run a DLL", "");
    }
    return true;
}

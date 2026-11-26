#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "syntax/syntax.h"

typedef struct {
    const char *source_path;
    const char *output_path;
    bool asm_only;
    bool keep_asm;
    bool check_only;
    bool read_stdin;
    bool run;
    bool dll;
    bool object;
    bool shared;
    bool symbols;
    bool help;
    const char *show_source;
    int window_program;
    bool optimize;
    bool zen3;
} Options;

typedef struct {
    const char *assembly;
    const char *definitions;
    const char *header;
} CompiledProgram;

typedef struct {
    Program *program;
    PointerList units;
    bool is_module;
} LoadedProgram;

bool parse_options(int argc, char **argv, Options *options);
char *replace_extension(const char *path, const char *extension);
char *read_source(const Options *options, size_t *length);
typedef struct {
    const char *name;
    const char *bits;
    const char *flip;
    const char *sum;
    bool is_integer;
} TemplateType;

extern const TemplateType TEMPLATE_TYPES[];
extern const int TEMPLATE_TYPE_COUNT;
char *instantiate_template(const char *text, const TemplateType *type);

extern PointerList import_folders;

LoadedProgram load_program(const char *main_path, const char *main_text, size_t main_length, bool allow_module);
CompiledProgram compile_source(const char *source_path, const char *text, size_t length, bool check_only, const char *dll_name,
                               int window_program, bool optimize, bool zen3, bool object);
void assemble_file(const char *asm_path, const char *output_path, bool object);
int link_shared_object(const char *object_path, const char *output_path, const char *soname);
void print_symbols(const Program *program);
bool print_embedded_source(const char *path);
LoadedProgram analyze_source(const char *source_path, const char *text, size_t length);

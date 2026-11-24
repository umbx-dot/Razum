#include <stdio.h>
#include <string.h>

#include "common/arena.h"
#include "common/diagnostics.h"
#include "common/files.h"
#include "driver/driver.h"
#include "platform/filesystem.h"
#include "platform/process.h"

static void write_output(const char *path, const char *text) {
    if (!write_file(path, text, strlen(text))) {
        fatal_error("cannot write %s", path);
    }
}

static const char *temporary_asm_path(const char *output_path) {
    size_t size = strlen(output_path) + 32;
    char *path = arena_alloc(size);
    snprintf(path, size, "%s.rzc-%ld.asm", output_path, process_id());
    return path;
}

static const char *file_name_of(const char *path) {
    const char *name = path;
    for (const char *cursor = path; *cursor != '\0'; cursor++) {
        if (*cursor == '/' || *cursor == '\\') {
            name = cursor + 1;
        }
    }
    return name;
}

static void check_output_paths(const Options *options, const char *const *outputs, size_t count) {
    for (size_t index = 0; index < count; index++) {
        const char *path = outputs[index];
        if (path == NULL) {
            continue;
        }
        if ((!options->read_stdin && strcmp(options->source_path, path) == 0) ||
            same_file(options->source_path, path)) {
            fatal_error("output %s would overwrite the source; choose a different -o path", path);
        }
        for (size_t other = 0; other < index; other++) {
            if (outputs[other] != NULL && (strcmp(outputs[other], path) == 0 || same_file(outputs[other], path))) {
                fatal_error("multiple outputs would overwrite %s; choose a different -o path", path);
            }
        }
    }
}

int main(int argc, char **argv) {
    Options options;
    if (!parse_options(argc, argv, &options)) {
        return 2;
    }
    if (options.help) {
        return 0;
    }
    if (options.show_source != NULL) {
        return print_embedded_source(options.show_source) ? 0 : 1;
    }
    size_t length = 0;
    const char *source = read_source(&options, &length);
    if (options.symbols) {
        print_symbols(analyze_source(options.source_path, source, length).program);
        return diagnostic_count > 0 ? 1 : 0;
    }
    if (options.dll && host_target() != TARGET_WINDOWS) {
        fatal_error("--dll makes a Windows DLL; Linux shared libraries are not supported yet");
    }
    const char *program_extension = host_target() == TARGET_WINDOWS ? ".exe" : "";
    const char *output_path = options.output_path != NULL ? options.output_path
                                                          : replace_extension(options.source_path, options.dll ? ".dll" : program_extension);
    CompiledProgram compiled = compile_source(options.source_path, source, length, options.check_only,
                                              options.dll ? file_name_of(output_path) : NULL, options.window_program,
                                              options.optimize, options.zen3);
    if (options.check_only) {
        return 0;
    }
    size_t output_length = strlen(output_path);
    bool names_asm = output_length > 4 && strcmp(output_path + output_length - 4, ".asm") == 0;
    bool keeps_asm = options.asm_only || options.keep_asm;
    const char *asm_path = options.asm_only && names_asm ? output_path
                         : keeps_asm                     ? replace_extension(output_path, ".asm")
                                                         : temporary_asm_path(output_path);
    const char *def_path = options.dll ? replace_extension(output_path, ".def") : NULL;
    const char *header_path = options.dll ? replace_extension(output_path, ".h") : NULL;
    const char *outputs[] = {asm_path, options.asm_only ? NULL : output_path, def_path, header_path};
    check_output_paths(&options, outputs, sizeof outputs / sizeof outputs[0]);
    write_output(asm_path, compiled.assembly);
    if (options.dll) {
        write_output(def_path, compiled.definitions);
        write_output(header_path, compiled.header);
    }
    if (options.asm_only) {
        return 0;
    }
    assemble_file(asm_path, output_path);
    if (!make_executable(output_path)) {
        fatal_error("cannot mark %s as executable", output_path);
    }
    if (!options.keep_asm) {
        remove(asm_path);
    }
    if (!options.run) {
        return 0;
    }
    const char *arguments[] = {output_path, NULL};
    int exit_code = run_process(arguments, NULL);
    if (exit_code < 0) {
        fatal_error("cannot start %s", output_path);
    }
    return exit_code;
}

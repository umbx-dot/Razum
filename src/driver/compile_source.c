#include "driver/driver.h"

#include <stdlib.h>

#include "analysis/analysis.h"
#include "checking/checking.h"
#include "codegen/codegen.h"
#include "common/diagnostics.h"

static bool has_export(const Program *program) {
    for (int index = 0; index < program->functions.count; index++) {
        if (LIST_AT(program->functions, Function, index)->is_exported) {
            return true;
        }
    }
    for (int index = 0; index < program->globals.count; index++) {
        if (LIST_AT(program->globals, GlobalVariable, index)->is_exported) {
            return true;
        }
    }
    return false;
}

CompiledProgram compile_source(const char *source_path, const char *text, size_t length, bool check_only, const char *dll_name,
                               int window_program, bool optimize, bool zen3, bool object) {
    CompiledProgram result = {0};
    LoadedProgram loaded = load_program(source_path, text, length, check_only || dll_name != NULL || object);
    if (diagnostic_count > 0) {
        exit(1);
    }
    check_program(loaded.program, !loaded.is_module && dll_name == NULL && !object);
    if (diagnostic_count > 0) {
        exit(1);
    }
    if (window_program != 0) {
        loaded.program->is_window_program = window_program > 0;
    }
    loaded.program->optimize = optimize;
    loaded.program->zen3 = zen3;
    bool has_dll_main = dll_name != NULL && check_dll_main(loaded.program);
    if (dll_name != NULL && !has_dll_main && !has_export(loaded.program)) {
        diagnostic_source_path = source_path;
        compile_error(1, 1, "a DLL needs an export or a DllMain entry point");
    }
    if (object && (!has_export(loaded.program) || loaded.program->is_window_program)) {
        diagnostic_source_path = source_path;
        compile_error(1, 1, "an object needs an export and cannot set a program subsystem");
    }
    if (check_only) {
        return result;
    }
    mark_used_functions(loaded.program);
    result.assembly = emit_program(loaded.program, &loaded.units, dll_name, has_dll_main, object);
    if (dll_name != NULL) {
        result.definitions = emit_def_file(loaded.program, dll_name);
    }
    if (dll_name != NULL || object) {
        result.header = emit_c_header(loaded.program, !object);
    }
    return result;
}

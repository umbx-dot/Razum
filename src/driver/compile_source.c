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
    return false;
}

CompiledProgram compile_source(const char *source_path, const char *text, size_t length, bool check_only, const char *dll_name,
                               int window_program, bool optimize, bool zen3) {
    CompiledProgram result = {0};
    LoadedProgram loaded = load_program(source_path, text, length, check_only || dll_name != NULL);
    if (diagnostic_count > 0) {
        exit(1);
    }
    check_program(loaded.program, !loaded.is_module && dll_name == NULL);
    if (diagnostic_count > 0) {
        exit(1);
    }
    if (window_program != 0) {
        loaded.program->is_window_program = window_program > 0;
    }
    loaded.program->optimize = optimize;
    loaded.program->zen3 = zen3;
    if (check_only) {
        return result;
    }
    if (dll_name != NULL && !has_export(loaded.program)) {
        diagnostic_source_path = source_path;
        compile_error(1, 1, "a DLL needs at least one function marked export, as in export int add(int a, int b) { ... }");
    }
    mark_used_functions(loaded.program);
    result.assembly = emit_program(loaded.program, &loaded.units, dll_name);
    if (dll_name != NULL) {
        result.definitions = emit_def_file(loaded.program, dll_name);
        result.header = emit_c_header(loaded.program);
    }
    return result;
}

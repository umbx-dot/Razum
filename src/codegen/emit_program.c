#include "codegen/codegen.h"

#include <string.h>

#include "analysis/analysis.h"
#include "common/diagnostics.h"
#include "linking/linking.h"

static void reserve_global_strings(ProgramContext *context, const Program *program) {
    for (int index = 0; index < program->globals.count; index++) {
        GlobalVariable *variable = LIST_AT(program->globals, GlobalVariable, index);
        if (variable->type == TYPE_STRING && (!variable->is_library || variable->symbol->is_used)) {
            const Expression *initial = variable->initial;
            variable->string_index = initial != NULL
                ? string_constant_index(context, initial->string.bytes, initial->string.length)
                : string_constant_index(context, "", 0);
        }
    }
}

static void emit_units(TextBuffer *output, const PointerList *units) {
    for (int index = 0; index < units->count; index++) {
        const AsmUnit *unit = LIST_AT(*units, AsmUnit, index);
        text_line(output, "");
        text_line(output, "; from %s", unit->origin);
        text_append(output, unit->text);
    }
}

static bool links(const PointerList *units, const char *label) {
    for (int index = 0; index < units->count; index++) {
        if (unit_defines(LIST_AT(*units, AsmUnit, index), label)) {
            return true;
        }
    }
    return false;
}

static bool main_takes_arguments(const Program *program) {
    for (int index = 0; index < program->functions.count; index++) {
        const Function *function = LIST_AT(program->functions, Function, index);
        if (!function->is_library && strcmp(function->name, "main") == 0) {
            return function->parameters.count > 0;
        }
    }
    return false;
}

static void require_no_externs(const Program *program) {
    for (int index = 0; index < program->functions.count; index++) {
        const Function *function = LIST_AT(program->functions, Function, index);
        if (function->is_used && function->extern_dll != NULL) {
            diagnostic_source_path = function->source_path;
            compile_error(function->line, function->column, "'%s' comes from %s.dll, which exists only on Windows; Linux programs cannot call it",
                          function->name, function->extern_dll);
        }
    }
}

static char *finish_linux_program(const Program *program, ProgramContext *context, const TextBuffer *functions, const LinkResult *link,
                                  bool takes_arguments) {
    if (link->imports.count > 0) {
        fatal_error("an asm block calls the Windows function %s, which Linux programs cannot call", LIST_AT(link->imports, const char, 0));
    }
    TextBuffer output = {0};
    emit_elf_header(&output);
    emit_linux_entry_point(&output, links(&link->units, "rz@flush"), takes_arguments, links(&link->units, "rz@environment"));
    text_append_bytes(&output, functions->data, functions->length);
    emit_units(&output, &link->units);
    emit_data_section(&output, context, program);
    if (links(&link->units, "rz@http_https_image")) {
        emit_elf_https_backend(&output);
    }
    text_line(&output, "");
    text_line(&output, "    align 8, db 0");
    text_line(&output, "elf@end:");
    return output.data;
}

char *emit_program(const Program *program, const PointerList *units, const char *dll_name, bool has_dll_main, bool object) {
    bool is_dll = dll_name != NULL;
    bool is_linux = program->target == TARGET_LINUX;
    if (is_linux) {
        require_no_externs(program);
    }
    ProgramContext context = {0};
    context.optimize = program->optimize;
    context.zen3 = program->zen3;
    context.elf_object = is_linux && object;
    TextBuffer functions = {0};
    for (int index = 0; index < program->functions.count; index++) {
        Function *function = LIST_AT(program->functions, Function, index);
        if (function->is_used && function->extern_dll == NULL) {
            emit_function(&context, function, &functions);
        }
    }
    reserve_global_strings(&context, program);
    bool takes_arguments = !is_dll && !object && main_takes_arguments(program);
    TextBuffer scanned = {0};
    text_line(&scanned, takes_arguments ? "    call rz@arguments" : "");
    text_append_bytes(&scanned, functions.data, functions.length);
    PointerList available = {0};
    static const AsmUnit NO_LOCK = {"<rzc>/one thread", "rz@lock_output:\nrz@unlock_output:\n    ret\n"};
    if (runs_on_one_thread(program, is_dll || object)) {
        list_push(&available, (void *)&NO_LOCK);
    }
    for (int index = 0; index < units->count; index++) {
        list_push(&available, units->items[index]);
    }
    LinkResult link = link_units(scanned.data, &available, !is_linux && !object);
    if (is_linux && !object) {
        return finish_linux_program(program, &context, &functions, &link, takes_arguments);
    }
    TextBuffer output = {0};
    if (object) {
        if (link.imports.count > 0) {
            fatal_error("--object cannot use Windows imports: %s; call OS functions in the native host", LIST_AT(link.imports, const char, 0));
        }
        if (context.elf_object) {
            if (links(&link.units, "rz@environment") || links(&link.units, "rz@http_https_image")) {
                fatal_error("this library requires the standalone Linux runtime entry point");
            }
            emit_elf_object_header(&output, program);
        } else {
            emit_coff_header(&output, program);
        }
        text_append_bytes(&output, functions.data, functions.length);
        emit_units(&output, &link.units);
        text_line(&output, "section .data align=8");
        emit_data_section(&output, &context, program);
        return output.data;
    }
    emit_pe_header(&output, is_dll, program->is_window_program);
    if (is_dll) {
        emit_dll_entry_point(&output, links(&link.units, "rz@flush"), has_dll_main);
    } else {
        emit_entry_point(&output, links(&link.units, "rz@flush"), takes_arguments);
    }
    text_append_bytes(&output, functions.data, functions.length);
    emit_units(&output, &link.units);
    emit_data_section(&output, &context, program);
    emit_import_section(&output, &link.imports);
    emit_export_section(&output, program, dll_name);
    emit_relocations(&output, &context, is_dll);
    text_line(&output, "");
    text_line(&output, "    align 4, db 0");
    text_line(&output, "pe@image_end:");
    return output.data;
}

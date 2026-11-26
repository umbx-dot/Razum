#include "checking/checking.h"

#include "common/diagnostics.h"

bool check_dll_main(const Program *program) {
    const Function *entry = find_function(program, "DllMain");
    if (entry == NULL || entry->is_library || entry->extern_dll != NULL) {
        return false;
    }
    if (entry->return_type == TYPE_INT && entry->parameters.count == 3) {
        const Parameter *module = LIST_AT(entry->parameters, Parameter, 0);
        const Parameter *reason = LIST_AT(entry->parameters, Parameter, 1);
        const Parameter *reserved = LIST_AT(entry->parameters, Parameter, 2);
        if (module->type == TYPE_LONG && reason->type == TYPE_UINT &&
            reserved->type == TYPE_POINTER && reserved->shape->element->kind == TYPE_VOID) {
            return true;
        }
    }
    diagnostic_source_path = entry->source_path;
    compile_error(entry->line, entry->column, "DllMain must be declared as int DllMain(long module, uint reason, void *reserved)");
}

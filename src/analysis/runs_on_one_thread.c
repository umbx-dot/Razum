#include "analysis/analysis.h"

bool runs_on_one_thread(const Program *program, bool is_dll) {
    if (is_dll) {
        return false;
    }
    for (int index = 0; index < program->functions.count; index++) {
        const Function *function = LIST_AT(program->functions, Function, index);
        if (!function->is_used || function->extern_dll != NULL) {
            continue;
        }
        if (function->address_taken || (!function->is_library && contains_asm(function->body))) {
            return false;
        }
    }
    return true;
}

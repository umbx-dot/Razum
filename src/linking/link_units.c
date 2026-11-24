#include "linking/linking.h"

#include <stddef.h>
#include <string.h>

#include "common/diagnostics.h"

static const AsmUnit *find_unit(const PointerList *units, const char *label) {
    for (int index = 0; index < units->count; index++) {
        const AsmUnit *unit = LIST_AT(*units, AsmUnit, index);
        if (unit_defines(unit, label)) {
            return unit;
        }
    }
    return NULL;
}

static bool includes(const PointerList *units, const AsmUnit *unit) {
    for (int index = 0; index < units->count; index++) {
        if (LIST_AT(*units, AsmUnit, index) == unit) {
            return true;
        }
    }
    return false;
}

static void collect_imports(const char *text, PointerList *imports) {
    PointerList references = {0};
    collect_references(text, "iat@", &references);
    for (int index = 0; index < references.count; index++) {
        const char *name = LIST_AT(references, const char, index) + 4;
        bool known = false;
        for (int existing = 0; existing < imports->count && !known; existing++) {
            known = strcmp(LIST_AT(*imports, const char, existing), name) == 0;
        }
        if (!known) {
            list_push(imports, (void *)name);
        }
    }
}

LinkResult link_units(const char *program_text, const PointerList *available_units, bool imports_exit) {
    LinkResult result = {0};
    PointerList labels = {0};
    collect_references(program_text, "rz@", &labels);
    for (int index = 0; index < labels.count; index++) {
        const char *label = LIST_AT(labels, const char, index);
        const AsmUnit *unit = find_unit(available_units, label);
        if (unit == NULL) {
            fatal_error("the program uses %s, but no library defines it", label);
        }
        if (!includes(&result.units, unit)) {
            list_push(&result.units, (void *)unit);
            collect_references(unit->text, "rz@", &labels);
        }
    }
    if (imports_exit) {
        list_push(&result.imports, "ExitProcess");
    }
    collect_imports(program_text, &result.imports);
    for (int index = 0; index < result.units.count; index++) {
        collect_imports(LIST_AT(result.units, AsmUnit, index)->text, &result.imports);
    }
    return result;
}

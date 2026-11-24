#include "checking/checking.h"

#include <inttypes.h>
#include <string.h>

#include "common/diagnostics.h"

static void check_members(const EnumType *enumeration) {
    for (int index = 0; index < enumeration->members.count; index++) {
        const EnumMember *member = LIST_AT(enumeration->members, EnumMember, index);
        if (member->value < INT32_MIN || member->value > INT32_MAX) {
            compile_error(member->line, member->column, "%s.%s would be %" PRId64 ", which does not fit an int", enumeration->name,
                          member->name, member->value);
        }
        for (int earlier = 0; earlier < index; earlier++) {
            if (strcmp(LIST_AT(enumeration->members, EnumMember, earlier)->name, member->name) == 0) {
                compile_error(member->line, member->column, "enum '%s' has two members named '%s'", enumeration->name, member->name);
            }
        }
    }
}

static bool is_struct_name(const Program *program, const char *name) {
    for (int index = 0; index < program->structs.count; index++) {
        if (strcmp(LIST_AT(program->structs, StructType, index)->name, name) == 0) {
            return true;
        }
    }
    return false;
}

void check_enums(Program *program) {
    for (int index = 0; index < program->enums.count; index++) {
        EnumType *enumeration = LIST_AT(program->enums, EnumType, index);
        diagnostic_source_path = enumeration->source_path;
        if (find_enum(program, enumeration->name) != enumeration) {
            compile_error(enumeration->line, enumeration->column, "enum '%s' is declared twice", enumeration->name);
        }
        if (is_struct_name(program, enumeration->name)) {
            compile_error(enumeration->line, enumeration->column, "'%s' is already the name of a struct", enumeration->name);
        }
        check_members(enumeration);
        enumeration->shape = new_shape(TYPE_INT, enumeration->line, enumeration->column);
        enumeration->shape->enumeration = enumeration;
    }
}

#include "checking/checking.h"

#include <string.h>

const EnumType *find_enum(const Program *program, const char *name) {
    for (int index = 0; index < program->enums.count; index++) {
        const EnumType *enumeration = LIST_AT(program->enums, EnumType, index);
        if (strcmp(enumeration->name, name) == 0) {
            return enumeration;
        }
    }
    return NULL;
}

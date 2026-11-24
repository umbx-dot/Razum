#include "checking/checking.h"

#include <string.h>

Function *find_function(const Program *program, const char *name) {
    for (int index = 0; index < program->functions.count; index++) {
        Function *function = LIST_AT(program->functions, Function, index);
        if (strcmp(function->name, name) == 0) {
            return function;
        }
    }
    return NULL;
}

#include "checking/checking.h"

#include <string.h>

#include "common/diagnostics.h"

typedef struct {
    const Program *program;
    int index;
} StructWork;

static void check_struct(void *data) {
    StructWork *work = data;
    const Program *program = work->program;
    int index = work->index;
    {
        StructType *structure = LIST_AT(program->structs, StructType, index);
        diagnostic_source_path = structure->source_path;
        for (int earlier = 0; earlier < index; earlier++) {
            StructType *other = LIST_AT(program->structs, StructType, earlier);
            if (strcmp(other->name, structure->name) == 0) {
                compile_error(structure->line, structure->column, "struct '%s' is already declared at %s:%d",
                              structure->name, other->source_path, other->line);
            }
        }
        layout_struct(program, structure);
    }
}

void check_structs(const Program *program) {
    for (int index = 0; index < program->structs.count; index++) {
        StructWork work = {program, index};
        diagnostic_try(check_struct, &work);
    }
}

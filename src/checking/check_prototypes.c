#include "checking/checking.h"

#include "common/diagnostics.h"

void separate_prototypes(Program *program) {
    int kept = 0;
    for (int index = 0; index < program->functions.count; index++) {
        Function *function = LIST_AT(program->functions, Function, index);
        if (function->is_prototype) {
            list_push(&program->prototypes, function);
        } else {
            program->functions.items[kept++] = function;
        }
    }
    program->functions.count = kept;
}

static bool same_signature_as(const Function *prototype, const Function *definition) {
    if (prototype->parameters.count != definition->parameters.count || !same_shape(prototype->return_shape, definition->return_shape)) {
        return false;
    }
    for (int index = 0; index < prototype->parameters.count; index++) {
        if (!same_shape(LIST_AT(prototype->parameters, Parameter, index)->shape, LIST_AT(definition->parameters, Parameter, index)->shape)) {
            return false;
        }
    }
    return true;
}

typedef struct {
    const Program *program;
    Function *prototype;
} PrototypeWork;

static void check_prototype(void *data) {
    PrototypeWork *work = data;
    Function *prototype = work->prototype;
    diagnostic_source_path = prototype->source_path;
    resolve_shape(work->program, prototype->return_shape);
    for (int index = 0; index < prototype->parameters.count; index++) {
        resolve_shape(work->program, LIST_AT(prototype->parameters, Parameter, index)->shape);
    }
    Function *definition = find_function(work->program, prototype->name);
    if (definition == NULL) {
        compile_error(prototype->line, prototype->column,
                      "'%s' is declared but has no body; razum functions need no declaration before use, so write the body here or remove this line",
                      prototype->name);
    }
    if (!same_signature_as(prototype, definition)) {
        compile_error(prototype->line, prototype->column, "this declaration of '%s' does not match its definition at %s:%d",
                      prototype->name, definition->source_path, definition->line);
    }
}

void check_prototypes(const Program *program) {
    for (int index = 0; index < program->prototypes.count; index++) {
        PrototypeWork work = {program, LIST_AT(program->prototypes, Function, index)};
        diagnostic_try(check_prototype, &work);
    }
}

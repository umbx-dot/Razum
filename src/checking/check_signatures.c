#include "checking/checking.h"

#include <string.h>

#include "common/diagnostics.h"

static bool is_global_name(const Program *program, const char *name) {
    for (int index = 0; index < program->globals.count; index++) {
        if (strcmp(LIST_AT(program->globals, GlobalVariable, index)->name, name) == 0) {
            return true;
        }
    }
    return false;
}

typedef struct {
    const Program *program;
    int index;
} SignatureWork;

static void check_signature(void *data) {
    SignatureWork *work = data;
    const Program *program = work->program;
    int index = work->index;
    {
        Function *function = LIST_AT(program->functions, Function, index);
        diagnostic_source_path = function->source_path;
        if (strcmp(function->name, "print") == 0 || strcmp(function->name, "free") == 0) {
            compile_error(function->line, function->column, "'%s' is built in and cannot be redefined", function->name);
        }
        resolve_shape(program, function->return_shape);
        function->return_type = function->return_shape->kind;
        if (function->return_type == TYPE_STRUCT) {
            compile_error(function->line, function->column, "a function cannot return a struct; pass one as a parameter and fill it in");
        }
        Function *first = find_function(program, function->name);
        if (first != function && function->is_library && !first->is_library) {
            diagnostic_source_path = first->source_path;
            compile_error(first->line, first->column, "'%s' is also defined by the library at %s:%d; rename yours or remove that import", function->name, function->source_path, function->line);
        }
        if (first != function) {
            compile_error(function->line, function->column, "'%s' is already declared at %s:%d", function->name, first->source_path, first->line);
        }
        if (is_global_name(program, function->name)) {
            compile_error(function->line, function->column, "'%s' is already the name of a global variable", function->name);
        }
        for (int parameter_index = 0; parameter_index < function->parameters.count; parameter_index++) {
            Parameter *parameter = LIST_AT(function->parameters, Parameter, parameter_index);
            resolve_shape(program, parameter->shape);
            parameter->type = parameter->shape->kind;
            if (parameter->type == TYPE_VOID) {
                compile_error(parameter->line, parameter->column, "a parameter cannot be void");
            }
        }
    }
}

void check_signatures(const Program *program, bool requires_main) {
    for (int index = 0; index < program->functions.count; index++) {
        SignatureWork work = {program, index};
        diagnostic_try(check_signature, &work);
    }
    if (!requires_main) {
        return;
    }
    Function *main = find_function(program, "main");
    diagnostic_source_path = program->main_path;
    if (main == NULL || main->is_library || main->extern_dll != NULL) {
        compile_error(1, 1, "a program needs an entry point: int main() { ... }");
    }
    diagnostic_source_path = main->source_path;
    const Parameter *arguments = main->parameters.count == 1 ? LIST_AT(main->parameters, Parameter, 0) : NULL;
    bool takes_arguments = arguments != NULL && arguments->shape->kind == TYPE_ARRAY && arguments->shape->count < 0 &&
                           arguments->shape->element->kind == TYPE_STRING;
    if (main->return_type != TYPE_INT || (main->parameters.count != 0 && !takes_arguments)) {
        compile_error(main->line, main->column, "main must be declared as int main() or int main(string args[])");
    }
}

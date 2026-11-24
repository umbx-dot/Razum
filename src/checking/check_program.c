#include "checking/checking.h"

#include "common/diagnostics.h"

typedef struct {
    Checker *checker;
    Function *function;
} FunctionWork;

static void check_function_work(void *data) {
    FunctionWork *work = data;
    check_function(work->checker, work->function);
}

void check_program(Program *program, bool requires_main) {
    Checker checker = {0};
    checker.program = program;
    separate_prototypes(program);
    check_enums(program);
    declare_globals(program);
    check_structs(program);
    check_globals(program);
    check_signatures(program, requires_main);
    check_prototypes(program);
    for (int index = 0; index < program->functions.count; index++) {
        FunctionWork work = {&checker, LIST_AT(program->functions, Function, index)};
        diagnostic_try(check_function_work, &work);
    }
    int kept = 0;
    for (int index = 0; index < program->globals.count; index++) {
        GlobalVariable *variable = LIST_AT(program->globals, GlobalVariable, index);
        if (variable->is_const) {
            list_push(&program->constants, variable);
        } else {
            program->globals.items[kept++] = variable;
        }
    }
    program->globals.count = kept;
}

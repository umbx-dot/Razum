#include "codegen/codegen.h"

#include <inttypes.h>

#include "common/arena.h"

static void emit_loop_body(FunctionContext *context, Statement *body, const char *continue_label, const char *break_label) {
    LoopLabels *labels = arena_alloc(sizeof *labels);
    labels->continue_label = continue_label;
    labels->break_label = break_label;
    list_push(&context->loops, labels);
    emit_statement(context, body);
    context->loops.count--;
}

static void emit_cached_loop_body(FunctionContext *context, Statement *body, const char *step, const char *end,
                                  LoopProducts *products, bool hoisted) {
    if (products->count == 0 && !hoisted) {
        emit_loop_body(context, body, step, end);
        return;
    }
    int last = 0;
    for (int i = 0; i < products->count; i++) {
        if (products->entries[i].last_statement > last) {
            last = products->entries[i].last_statement;
        }
    }
    LoopLabels labels = {step, end};
    list_push(&context->loops, &labels);
    for (int i = hoisted ? 1 : 0; i < body->block.count; i++) {
        products->statement = i;
        context->loop_products = i <= last ? products : NULL;
        emit_statement(context, LIST_AT(body->block, Statement, i));
    }
    context->loop_products = NULL;
    context->loops.count--;
}

static bool emit_invariant_prefix(FunctionContext *context, const Statement *body, const Statement *step,
                                  Expression *condition, const char *end) {
    Statement *invariant = loop_invariant_declaration(body, step);
    if (invariant == NULL) {
        return false;
    }
    if (condition != NULL) {
        jump_if(context, condition, end, false);
    }
    emit_statement(context, invariant);
    return true;
}

static void emit_if(FunctionContext *context, Statement *statement) {
    if (context->program->optimize && emit_mask_select(context, statement)) {
        return;
    }
    const char *end_label = new_label(context, "endif");
    Statement *else_branch = statement->if_statement.else_branch;
    const char *else_label = else_branch != NULL ? new_label(context, "else") : end_label;
    jump_if(context, statement->if_statement.condition, else_label, false);
    emit_statement(context, statement->if_statement.then_branch);
    if (else_branch != NULL) {
        emit(context, "jmp %s", end_label);
        emit_label(context, else_label);
        emit_statement(context, else_branch);
    }
    emit_label(context, end_label);
}

static void emit_while(FunctionContext *context, Statement *statement) {
    const char *top_label = new_label(context, "while");
    const char *end_label = new_label(context, "endwhile");
    if (context->program->optimize) {
        LoopProducts products = {0};
        Expression *condition = statement->while_statement.condition;
        Statement *body = statement->while_statement.body;
        plan_loop_products(context, condition, body, &products);
        const char *test_label = new_label(context, "whiletest");
        bool hoisted = products.count == 0 && emit_invariant_prefix(context, body, NULL, condition, end_label);
        if (!hoisted) {
            emit(context, "jmp %s", test_label);
        }
        emit_label(context, top_label);
        emit_cached_loop_body(context, body, test_label, end_label, &products, hoisted);
        emit_label(context, test_label);
        emit_cached_loop_test(context, condition, top_label, end_label, &products);
        emit_label(context, end_label);
        return;
    }
    emit_label(context, top_label);
    jump_if(context, statement->while_statement.condition, end_label, false);
    emit_loop_body(context, statement->while_statement.body, top_label, end_label);
    emit(context, "jmp %s", top_label);
    emit_label(context, end_label);
}

static void emit_for(FunctionContext *context, Statement *statement) {
    if (statement->for_statement.initial != NULL) {
        emit_simple_statement(context, statement->for_statement.initial);
    }
    if (context->program->optimize && !emit_packed_double_loop_prefix(context, statement)) {
        emit_vector_loop_prefix(context, statement);
    }
    const char *top_label = new_label(context, "for");
    const char *step_label = new_label(context, "step");
    const char *end_label = new_label(context, "endfor");
    Expression *condition = statement->for_statement.condition;
    Statement *body = statement->for_statement.body;
    Statement *step = statement->for_statement.step;
    const char *test_label = NULL;
    LoopProducts products = {0};
    bool hoisted = false;
    if (context->program->optimize) {
        plan_loop_products(context, condition, body, &products);
        if (condition != NULL) {
            test_label = new_label(context, "fortest");
        }
        hoisted = products.count == 0 && emit_invariant_prefix(context, body, step, condition, end_label);
        if (!hoisted && test_label != NULL) {
            emit(context, "jmp %s", test_label);
        }
    }
    emit_label(context, top_label);
    if (condition != NULL && test_label == NULL) {
        jump_if(context, condition, end_label, false);
    }
    emit_cached_loop_body(context, body, step_label, end_label, &products, hoisted);
    emit_label(context, step_label);
    if (step != NULL) {
        emit_simple_statement(context, step);
    }
    if (test_label != NULL) {
        emit_label(context, test_label);
        emit_cached_loop_test(context, condition, top_label, end_label, &products);
    } else {
        emit(context, "jmp %s", top_label);
    }
    emit_label(context, end_label);
}

static void emit_do(FunctionContext *context, Statement *statement) {
    const char *top_label = new_label(context, "do");
    const char *test_label = new_label(context, "test");
    const char *end_label = new_label(context, "enddo");
    emit_label(context, top_label);
    emit_loop_body(context, statement->while_statement.body, test_label, end_label);
    emit_label(context, test_label);
    jump_if(context, statement->while_statement.condition, top_label, true);
    emit_label(context, end_label);
}

static void emit_case_tests(FunctionContext *context, const SwitchCase *switch_case, const char *label) {
    for (int index = 0; index < switch_case->values.count; index++) {
        int64_t value = LIST_AT(switch_case->values, Expression, index)->int_literal.value;
        if (value >= INT32_MIN && value <= INT32_MAX) {
            emit(context, "cmp rax, %" PRId64, value);
        } else {
            emit(context, "mov rcx, %" PRId64, value);
            emit(context, "cmp rax, rcx");
        }
        emit(context, "je %s", label);
    }
}

static void emit_switch(FunctionContext *context, Statement *statement) {
    const PointerList *cases = &statement->switch_statement.cases;
    const char *end_label = new_label(context, "endswitch");
    const char **labels = arena_alloc(sizeof *labels * (size_t)(cases->count > 0 ? cases->count : 1));
    const char *default_label = end_label;
    emit_int(context, statement->switch_statement.value);
    for (int index = 0; index < cases->count; index++) {
        const SwitchCase *switch_case = LIST_AT(*cases, SwitchCase, index);
        labels[index] = new_label(context, switch_case->is_default ? "default" : "case");
        if (switch_case->is_default) {
            default_label = labels[index];
        }
        emit_case_tests(context, switch_case, labels[index]);
    }
    emit(context, "jmp %s", default_label);
    const char *continue_label = context->loops.count > 0 ? LIST_AT(context->loops, LoopLabels, context->loops.count - 1)->continue_label : NULL;
    for (int index = 0; index < cases->count; index++) {
        emit_label(context, labels[index]);
        emit_loop_body(context, LIST_AT(*cases, SwitchCase, index)->body, continue_label, end_label);
        if (index + 1 < cases->count) {
            emit(context, "jmp %s", end_label);
        }
    }
    emit_label(context, end_label);
}

void emit_control(FunctionContext *context, Statement *statement) {
    switch (statement->kind) {
        case STATEMENT_DO:
            emit_do(context, statement);
            return;
        case STATEMENT_SWITCH:
            emit_switch(context, statement);
            return;
        case STATEMENT_IF:
            emit_if(context, statement);
            return;
        case STATEMENT_WHILE:
            emit_while(context, statement);
            return;
        default:
            emit_for(context, statement);
            return;
    }
}

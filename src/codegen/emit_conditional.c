#include "codegen/codegen.h"

void emit_conditional(FunctionContext *context, Expression *expression, bool is_floating_result) {
    if (context->program->optimize && !is_floating_result && emit_conditional_move(context, expression)) {
        return;
    }
    const char *else_label = new_label(context, "else");
    const char *end_label = new_label(context, "endif");
    jump_if(context, expression->conditional.condition, else_label, false);
    if (is_floating_result) {
        emit_double(context, expression->conditional.when_true);
    } else {
        emit_int(context, expression->conditional.when_true);
    }
    emit(context, "jmp %s", end_label);
    emit_label(context, else_label);
    if (is_floating_result) {
        emit_double(context, expression->conditional.when_false);
    } else {
        emit_int(context, expression->conditional.when_false);
    }
    emit_label(context, end_label);
}

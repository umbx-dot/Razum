#include "codegen/codegen.h"

#include <stdio.h>

#include "common/diagnostics.h"

const char *double_arithmetic_instruction(Operator operator) {
    switch (operator) {
        case OP_ADD: return "addsd";
        case OP_SUBTRACT: return "subsd";
        case OP_MULTIPLY: return "mulsd";
        default: return "divsd";
    }
}

Operand emit_double_pair(FunctionContext *context, Expression *left, Expression *right) {
    Operand operand = {0};
    bool simple = right->type == TYPE_DOUBLE && double_operand(context->program, right, &operand);
    emit_double(context, left);
    if (simple) {
        return operand;
    }
    if (right->kind == EXPRESSION_NAME && right->type == TYPE_FLOAT) {
        emit_load_floating(context, right->name.symbol, "xmm1");
        snprintf(operand.text, sizeof operand.text, "xmm1");
        return operand;
    }
    if (context->program->optimize && emit_double_tree(context, right, "xmm1")) {
        snprintf(operand.text, sizeof operand.text, "xmm1");
        return operand;
    }
    Operand temp = acquire_temp(context);
    emit(context, "movsd %s, xmm0", temp.text);
    emit_double(context, right);
    emit(context, "movsd xmm1, xmm0");
    emit(context, "movsd xmm0, %s", temp.text);
    release_temp(context);
    snprintf(operand.text, sizeof operand.text, "xmm1");
    return operand;
}

void emit_value(FunctionContext *context, Expression *expression, ValueType target) {
    if (is_floating(target)) {
        emit_double(context, expression);
        if (target == TYPE_FLOAT) {
            emit(context, "cvtsd2ss xmm0, xmm0");
        }
    } else {
        emit_int(context, expression);
    }
}

void emit_double(FunctionContext *context, Expression *expression) {
    if (!is_floating(expression->type)) {
        emit_int(context, expression);
        emit_int_to_double(context, expression->type);
        return;
    }
    switch (expression->kind) {
        case EXPRESSION_DOUBLE:
            emit(context, "movsd xmm0, qword [num@%d]", number_constant_index(context->program, expression->double_literal));
            return;
        case EXPRESSION_NAME:
            emit_load_floating(context, expression->name.symbol, "xmm0");
            return;
        case EXPRESSION_INDEX:
        case EXPRESSION_FIELD:
        case EXPRESSION_DEREF:
            emit_place_load(context, expression);
            return;
        case EXPRESSION_CONDITIONAL:
            emit_conditional(context, expression, true);
            return;
        case EXPRESSION_CALL:
            emit_call(context, expression);
            if (expression->type == TYPE_FLOAT) {
                emit(context, "cvtss2sd xmm0, xmm0");
            }
            return;
        case EXPRESSION_CAST:
            emit_double(context, expression->cast.operand);
            if (expression->type == TYPE_FLOAT && expression->cast.operand->type != TYPE_FLOAT) {
                emit(context, "cvtsd2ss xmm0, xmm0");
                emit(context, "cvtss2sd xmm0, xmm0");
            }
            return;
        case EXPRESSION_UNARY:
            emit_double(context, expression->unary.operand);
            emit(context, "movq rax, xmm0");
            emit(context, "btc rax, 63");
            emit(context, "movq xmm0, rax");
            return;
        case EXPRESSION_BINARY: {
            if (context->program->optimize && emit_double_tree(context, expression, "xmm0")) {
                return;
            }
            Operand operand = emit_double_pair(context, expression->binary.left, expression->binary.right);
            emit(context, "%s xmm0, %s", double_arithmetic_instruction(expression->binary.operator), operand.text);
            return;
        }
        default:
            fatal_error("internal: expression kind %d has no double value", expression->kind);
    }
}

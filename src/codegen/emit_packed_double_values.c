#include "codegen/packed_double_loop.h"

#include <stdlib.h>

static void broadcast_double(FunctionContext *context, bool avx, const char *source, int target) {
    if (avx) {
        emit(context, "vbroadcastsd ymm%d, %s", target, source);
    } else {
        emit(context, "movsd xmm%d, %s", target, source);
        emit(context, "unpcklpd xmm%d, xmm%d", target, target);
    }
}

static void broadcast_literal(FunctionContext *context, bool avx, double value, int target) {
    if (double_bits(value) == 0) {
        if (avx) {
            emit(context, "vxorpd ymm%d, ymm%d, ymm%d", target, target, target);
        } else {
            emit(context, "xorpd xmm%d, xmm%d", target, target);
        }
    } else {
        Operand source;
        (void)double_operand(context->program, &(Expression){.kind = EXPRESSION_DOUBLE, .double_literal = value}, &source);
        broadcast_double(context, avx, source.text, target);
    }
}

static void broadcast_symbol(FunctionContext *context, bool avx, const Symbol *symbol, int target, const Operand *saved) {
    if (symbol->type == TYPE_INT) {
        Operand source = memory_operand(symbol);
        if (avx) {
            emit(context, "vcvtsi2sd xmm%d, xmm%d, %s", target, target, source.text);
            emit(context, "vbroadcastsd ymm%d, xmm%d", target, target);
        } else {
            emit(context, "cvtsi2sd xmm%d, %s", target, source.text);
            emit(context, "unpcklpd xmm%d, xmm%d", target, target);
        }
        return;
    }
    Operand source = memory_operand(symbol);
    if (symbol->register_name != NULL) {
        const Operand *slot = &saved[atoi(symbol->register_name + 3) - FIRST_SAVED_DOUBLE_VECTOR];
        if (slot->text[0] != '\0') {
            source = *slot;
        }
    }
    broadcast_double(context, avx, source.text, target);
}

static void packed_binary(FunctionContext *context, bool avx, Operator op, int target, int left, int right) {
    const char *instruction = op == OP_ADD ? "addpd" : op == OP_SUBTRACT ? "subpd" :
                              op == OP_MULTIPLY ? "mulpd" : "divpd";
    if (avx) {
        emit(context, "v%s ymm%d, ymm%d, ymm%d", instruction, target, left, right);
    } else {
        if (target != left) {
            emit(context, "movapd xmm%d, xmm%d", target, left);
        }
        emit(context, "%s xmm%d, xmm%d", instruction, target, right);
    }
}

static void emit_lane_indices(FunctionContext *context, const PackedDoubleLoop *loop, int target, const char *lanes) {
    emit(context, "mov rax, %s", loop->index->register_name);
    if (loop->avx) {
        emit(context, "vmovd xmm%d, eax", target);
        emit(context, "vpshufd xmm%d, xmm%d, 0", target, target);
        emit(context, "vpaddd xmm%d, xmm%d, [rel %s]", target, target, lanes);
        emit(context, "vcvtdq2pd ymm%d, xmm%d", target, target);
    } else {
        emit(context, "movd xmm%d, eax", target);
        emit(context, "pshufd xmm%d, xmm%d, 0", target, target);
        emit(context, "paddd xmm%d, [rel %s]", target, lanes);
        emit(context, "cvtdq2pd xmm%d, xmm%d", target, target);
    }
}

static void initialize(FunctionContext *context, const PackedDoubleLoop *loop, const Expression *value,
                       int target, int scratch, const Operand *saved, const char *lanes) {
    switch (value->kind) {
        case EXPRESSION_DOUBLE:
            broadcast_literal(context, loop->avx, value->double_literal, target);
            return;
        case EXPRESSION_INT:
            broadcast_literal(context, loop->avx, (double)value->int_literal.value, target);
            return;
        case EXPRESSION_NAME: {
            const Symbol *symbol = value->name.symbol;
            if (symbol == loop->index) {
                emit_lane_indices(context, loop, target, lanes);
                return;
            }
            for (int i = 0; i < loop->variable_count; i++) {
                if (loop->variables[i].symbol == symbol) {
                    if (i != target) {
                        emit(context, loop->avx ? "vmovapd ymm%d, ymm%d" : "movapd xmm%d, xmm%d", target, i);
                    }
                    return;
                }
            }
            broadcast_symbol(context, loop->avx, symbol, target, saved);
            return;
        }
        default:
            initialize(context, loop, value->binary.left, target, scratch, saved, lanes);
            initialize(context, loop, value->binary.right, scratch, scratch + 1, saved, lanes);
            packed_binary(context, loop->avx, value->binary.operator, target, target, scratch);
            return;
    }
}

void emit_packed_double_inputs(FunctionContext *context, const PackedDoubleLoop *loop,
                               const Operand *saved, const char *lanes) {
    for (int i = 0; i < loop->variable_count; i++) {
        initialize(context, loop, loop->variables[i].initial, i, loop->variable_count, saved, lanes);
    }
    for (int i = loop->variable_count; i < loop->value_count; i++) {
        const Expression *input = loop->values[i].input;
        if (input == NULL) {
            continue;
        }
        if (input->kind == EXPRESSION_NAME) {
            broadcast_symbol(context, loop->avx, input->name.symbol, i, saved);
        } else {
            broadcast_literal(context, loop->avx, input->double_literal, i);
        }
    }
}

void emit_packed_double_operations(FunctionContext *context, const PackedDoubleLoop *loop, int first, int end) {
    for (int i = first; i < end; i++) {
        const PackedDoubleValue *value = &loop->values[i];
        if (value->operator != OP_ASSIGN) {
            packed_binary(context, loop->avx, value->operator, i, value->left, value->right);
        }
    }
}

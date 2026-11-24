#include "codegen/vector_loop.h"

#include <inttypes.h>

void emit_vector_binary(FunctionContext *context, const VectorLoop *loop, const char *instruction, int target, int right) {
    if (loop->avx) {
        emit(context, "v%s ymm%d, ymm%d, ymm%d", instruction, target, target, right);
    } else {
        emit(context, "%s xmm%d, xmm%d", instruction, target, right);
    }
}

void emit_vector_broadcast(FunctionContext *context, const VectorLoop *loop, int target, const char *source) {
    if (loop->avx) {
        emit(context, "vmovq xmm%d, %s", target, source);
        emit(context, "vpbroadcast%c ymm%d, xmm%d", loop->width == 8 ? 'q' : 'd', target, target);
    } else {
        emit(context, "movq xmm%d, %s", target, source);
        if (loop->width == 8) {
            emit(context, "punpcklqdq xmm%d, xmm%d", target, target);
        } else {
            emit(context, "pshufd xmm%d, xmm%d, 0", target, target);
        }
    }
}

static void copy_vector(FunctionContext *context, const VectorLoop *loop, int target, int source) {
    if (loop->avx) {
        emit(context, "vmovdqa ymm%d, ymm%d", target, source);
    } else {
        emit(context, "movdqa xmm%d, xmm%d", target, source);
    }
}

static void shift_vector(FunctionContext *context, const VectorLoop *loop, const char *instruction, int target, unsigned count) {
    if (loop->avx) {
        emit(context, "v%s ymm%d, ymm%d, %u", instruction, target, target, count);
    } else {
        emit(context, "%s xmm%d, %u", instruction, target, count);
    }
}

void emit_vector_access(FunctionContext *context, const VectorLoop *loop, const Symbol *symbol, int target, bool store) {
    const char *reg = symbol->register_name;
    if (reg == NULL) {
        if (!symbol->is_reference) {
            emit(context, "lea r10,%s", sized_operand(symbol, "").text);
        } else {
            emit(context, "mov r10, %s", memory_operand(symbol).text);
        }
        reg = "r10";
    }
    const char *instruction = loop->avx ? "vmovdqu" : "movdqu";
    const char *vec = loop->avx ? "ymm" : "xmm";
    if (store) {
        emit(context, "%s [%s + %s*%d], %s%d", instruction, reg, loop->index->register_name, loop->width, vec, target);
    } else {
        emit(context, "%s %s%d, [%s + %s*%d]", instruction, vec, target, reg, loop->index->register_name, loop->width);
    }
}

static void emit_shift(FunctionContext *context, const VectorLoop *loop, const Expression *value, int target) {
    const Expression *left = value->binary.left;
    unsigned count = (unsigned)value->binary.right->int_literal.value & (unsigned)(loop->width * 8 - 1);
    if (count == 0) {
        return;
    }
    bool is_signed = type_info(value->type)->is_signed &&
                     !(loop->nonnegative_index && left->kind == EXPRESSION_NAME && left->name.symbol == loop->index);
    if (value->binary.operator == OP_SHIFT_LEFT) {
        shift_vector(context, loop, loop->width == 8 ? "psllq" : "pslld", target, count);
    } else if (loop->width == 4) {
        shift_vector(context, loop, is_signed ? "psrad" : "psrld", target, count);
    } else {
        shift_vector(context, loop, "psrlq", target, count);
        if (is_signed) {
            emit(context, "mov r10, %" PRIu64, UINT64_C(1) << (63 - count));
            emit_vector_broadcast(context, loop, 5, "r10");
            emit_vector_binary(context, loop, "pxor", target, 5);
            emit_vector_binary(context, loop, "psubq", target, 5);
        }
    }
}

static void emit_square(FunctionContext *context, const VectorLoop *loop, int target) {
    if (!loop->index_fits_u32) {
        copy_vector(context, loop, 5, 1);
        shift_vector(context, loop, "psrlq", 5, 32);
        emit_vector_binary(context, loop, "pmuludq", 5, 1);
        shift_vector(context, loop, "psllq", 5, 33);
    }
    if (loop->avx) {
        emit(context, "vpmuludq ymm%d, ymm1, ymm1", target);
    } else {
        copy_vector(context, loop, target, 1);
        emit_vector_binary(context, loop, "pmuludq", target, 1);
    }
    if (!loop->index_fits_u32) {
        emit_vector_binary(context, loop, "paddq", target, 5);
    }
}

static const char *binary_instruction(Operator op, int width) {
    switch (op) {
        case OP_ADD: return width == 8 ? "paddq" : "paddd";
        case OP_SUBTRACT: return width == 8 ? "psubq" : "psubd";
        case OP_BIT_AND: return "pand";
        case OP_BIT_OR: return "por";
        case OP_BIT_XOR: return "pxor";
        default: return "pmulld";
    }
}

static void multiply_dwords_sse2(FunctionContext *context, int target, int right) {
    emit(context, "movdqa xmm5, xmm%d", target);
    emit(context, "pmuludq xmm%d, xmm%d", target, right);
    emit(context, "psrlq xmm5, 32");
    emit(context, "psrlq xmm%d, 32", right);
    emit(context, "pmuludq xmm5, xmm%d", right);
    emit(context, "pshufd xmm%d, xmm%d, 0x88", target, target);
    emit(context, "pshufd xmm5, xmm5, 0x88");
    emit(context, "punpckldq xmm%d, xmm5", target);
}

void emit_vector_expression(FunctionContext *context, const VectorLoop *loop, const Expression *value, int target) {
    switch (value->kind) {
        case EXPRESSION_INT:
            emit(context, "mov r10, %" PRId64, value->int_literal.value);
            emit_vector_broadcast(context, loop, target, "r10");
            return;
        case EXPRESSION_NAME:
            if (value->name.symbol == loop->index) {
                copy_vector(context, loop, target, 1);
            } else {
                emit_vector_broadcast(context, loop, target, value->name.symbol->register_name);
            }
            return;
        case EXPRESSION_INDEX:
            emit_vector_access(context, loop, value->index.base->name.symbol, target, false);
            return;
        default:
            break;
    }
    Operator op = value->binary.operator;
    if (op == OP_MULTIPLY && loop->width == 8) {
        emit_square(context, loop, target);
        return;
    }
    emit_vector_expression(context, loop, value->binary.left, target);
    if (is_shift(op)) {
        emit_shift(context, loop, value, target);
    } else {
        emit_vector_expression(context, loop, value->binary.right, target + 1);
        if (!loop->avx && op == OP_MULTIPLY) {
            multiply_dwords_sse2(context, target, target + 1);
        } else {
            emit_vector_binary(context, loop, binary_instruction(op, loop->width), target, target + 1);
        }
    }
}

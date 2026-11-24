#include "codegen/vector_loop.h"

static void reduce(FunctionContext *context, const VectorLoop *loop) {
    const char *add = loop->width == 8 ? "paddq" : "paddd";
    if (loop->avx) {
        emit(context, "vextracti128 xmm2, ymm0, 1");
        emit(context, "v%s xmm0, xmm0, xmm2", add);
        emit(context, "vpshufd xmm2, xmm0, 0x4e");
        emit(context, "v%s xmm0, xmm0, xmm2", add);
        if (loop->width == 4) {
            emit(context, "vpshufd xmm2, xmm0, 0xb1");
            emit(context, "vpaddd xmm0, xmm0, xmm2");
        }
        emit(context, "%s, xmm0", loop->width == 8 ? "vmovq rax" : "vmovd eax");
        emit(context, "vzeroupper");
    } else {
        emit(context, "pshufd xmm2, xmm0, 0x4e");
        emit(context, "%s xmm0, xmm2", add);
        if (loop->width == 4) {
            emit(context, "pshufd xmm2, xmm0, 0xb1");
            emit(context, "paddd xmm0, xmm2");
        }
        emit(context, "%s, xmm0", loop->width == 8 ? "movq rax" : "movd eax");
    }
    emit(context, "add rax, %s", loop->sum->register_name);
    emit_narrow(context, loop->sum->type);
    emit(context, "mov %s, rax", loop->sum->register_name);
}

static void emit_lane_data(FunctionContext *context, const VectorLoop *loop, const char *initial, const char *step, const char *scalar) {
    const char *size = loop->width == 8 ? "dq" : "dd";
    emit(context, "jmp %s", scalar);
    emit(context, "align %d, db 0", loop->lanes * loop->width);
    emit_label(context, initial);
    for (int i = 0; i < loop->lanes; i++) {
        emit(context, "%s %d", size, i);
    }
    emit_label(context, step);
    emit(context, "times %d %s %d", loop->lanes, size, loop->lanes);
}

void emit_vector_loop_prefix(FunctionContext *context, const Statement *statement) {
    VectorLoop loop = {0};
    if (!plan_vector_loop(&loop, statement, context->program->zen3)) {
        return;
    }
    const char *top = new_label(context, "vector");
    const char *scalar = new_label(context, "scalar");
    const char *initial = loop.uses_index ? new_label(context, "vectorlanes") : NULL;
    const char *step = loop.uses_index ? new_label(context, "vectorstep") : NULL;
    const char *index = loop.index->register_name;
    const char *add = loop.width == 8 ? "paddq" : "paddd";
    const char *vec = loop.avx ? "ymm" : "xmm";
    emit_int(context, loop.bound);
    emit(context, "cmp %s, rax", index);
    emit(context, "%s %s", loop.comparison == TYPE_ULONG ? "jae" : "jge", scalar);
    emit(context, "sub rax, %s", index);
    emit(context, "and rax, -%d", loop.lanes);
    emit(context, "jz %s", scalar);
    emit(context, "mov r11, rax");
    if (loop.sum != NULL) {
        emit(context, "%s", loop.avx ? "vpxor ymm0, ymm0, ymm0" : "pxor xmm0, xmm0");
    }
    if (loop.uses_index) {
        emit_vector_broadcast(context, &loop, 1, index);
        emit(context, "%s%s %s1, %s[rel %s]", loop.avx ? "v" : "", add, vec, loop.avx ? "ymm1, " : "", initial);
    }
    emit_label(context, top);
    emit_vector_expression(context, &loop, loop.value, 2);
    if (loop.sum != NULL) {
        emit_vector_binary(context, &loop, add, 0, 2);
    } else {
        emit_vector_access(context, &loop, loop.store_base, 2, true);
    }
    emit(context, "add %s, %d", index, loop.lanes);
    if (loop.uses_index) {
        emit(context, "%s%s %s1, %s[rel %s]", loop.avx ? "v" : "", add, vec, loop.avx ? "ymm1, " : "", step);
    }
    emit(context, "sub r11, %d", loop.lanes);
    emit(context, "jnz %s", top);
    if (loop.sum != NULL) {
        reduce(context, &loop);
    } else if (loop.avx) {
        emit(context, "vzeroupper");
    }
    if (loop.uses_index) {
        emit_lane_data(context, &loop, initial, step, scalar);
    }
    emit_label(context, scalar);
}

#include "codegen/packed_double_loop.h"

enum { FLOAT_EXCEPTION_MASKS = 0x1f80, AVX_QUIET_PREDICATE = 16 };

static void emit_activity_test(FunctionContext *context, const PackedDoubleLoop *loop, const char *body,
                               const char *done, const char *restore) {
    int comparison = loop->comparison;
    int mask = loop->value_count;
    emit_packed_double_operations(context, loop, loop->variable_count, comparison);
    if (loop->avx) {
        emit(context, "vcmppd ymm%d, ymm%d, ymm%d, %d", comparison, loop->compare_left, loop->compare_right,
             loop->compare_predicate | AVX_QUIET_PREDICATE);
        emit(context, "vmovmskpd eax, ymm%d", comparison);
    } else {
        emit(context, "movapd xmm%d, xmm%d", comparison, loop->compare_left);
        emit(context, "cmpunordpd xmm%d, xmm%d", comparison, loop->compare_right);
        emit(context, "movmskpd eax, xmm%d", comparison);
        emit(context, "test eax, eax");
        emit(context, "jnz %s", restore);
        emit(context, "movapd xmm%d, xmm%d", comparison, loop->compare_left);
        emit(context, "cmppd xmm%d, xmm%d, %d", comparison, loop->compare_right, loop->compare_predicate);
        emit(context, "movmskpd eax, xmm%d", comparison);
    }
    emit(context, "and eax, r8d");
    emit(context, "jz %s", done);
    emit(context, "cmp eax, r8d");
    emit(context, "je %s", body);
    emit(context, "mov r8d, eax");
    if (loop->avx) {
        emit(context, "vandpd ymm%d, ymm%d, ymm%d", mask, mask, comparison);
    } else {
        emit(context, "andpd xmm%d, xmm%d", mask, comparison);
    }
    for (int i = 0; i < loop->value_count; i++) {
        if (i < comparison || loop->values[i].input != NULL) {
            if (loop->avx) {
                emit(context, "vandpd ymm%d, ymm%d, ymm%d", i, i, mask);
            } else {
                emit(context, "andpd xmm%d, xmm%d", i, mask);
            }
        }
    }
}

static void emit_step(FunctionContext *context, const PackedDoubleLoop *loop) {
    emit_packed_double_operations(context, loop, loop->comparison + 1, loop->value_count);
    for (int i = 0; i < loop->variable_count; i++) {
        int next = loop->variables[i].next;
        if (next != i) {
            emit(context, loop->avx ? "vmovapd ymm%d, ymm%d" : "movapd xmm%d, xmm%d", i, next);
        }
    }
}

bool emit_packed_double_loop_prefix(FunctionContext *context, const Statement *statement) {
    PackedDoubleLoop loop = {.avx = context->program->zen3};
    if (!plan_packed_double_loop(&loop, statement)) {
        return false;
    }
    int width = loop.avx ? 4 : 2;
    const char *scalar = new_label(context, "doublescalar");
    const char *group = new_label(context, "doublegroup");
    const char *test = new_label(context, "doubletest");
    const char *body = new_label(context, "doublebody");
    const char *done = new_label(context, "doubledone");
    const char *restore = loop.avx ? NULL : new_label(context, "doublerestore");
    const char *lanes = new_label(context, "doublelanes");
    const char *index = loop.index->register_name;
    const char *total = loop.total->register_name;
    emit_int(context, loop.bound);
    emit(context, "sub rax, %s", index);
    emit(context, "cmp rax, %d", width);
    emit(context, "jl %s", scalar);
    emit(context, "shr rax, %d", loop.avx ? 2 : 1);
    emit(context, "mov r10, rax");
    emit_int(context, loop.limit);
    emit(context, "test eax, eax");
    emit(context, "jle %s", scalar);
    emit(context, "mov r11d, eax");
    Operand mxcsr = acquire_temp(context);
    emit(context, "stmxcsr dword %s", mxcsr.text + 6);
    emit(context, "mov eax, dword %s", mxcsr.text + 6);
    emit(context, "and eax, 0x%x", FLOAT_EXCEPTION_MASKS);
    emit(context, "cmp eax, 0x%x", FLOAT_EXCEPTION_MASKS);
    emit(context, "jne %s", scalar);
    release_temp(context);
    int mask = loop.value_count;
    int registers = loop.init_register_count > mask ? loop.init_register_count : mask + 1;
    Operand saved[PACKED_DOUBLE_REGISTERS - FIRST_SAVED_DOUBLE_VECTOR] = {0};
    for (int i = FIRST_SAVED_DOUBLE_VECTOR; i < registers; i++) {
        Operand slot = acquire_temp(context);
        (void)acquire_temp(context);
        saved[i - FIRST_SAVED_DOUBLE_VECTOR] = slot;
        emit(context, "movups %s, xmm%d", slot.text + 6, i);
    }
    emit_label(context, group);
    emit_packed_double_inputs(context, &loop, saved, lanes);
    if (loop.avx) {
        emit(context, "vpcmpeqd ymm%d, ymm%d, ymm%d", mask, mask, mask);
    } else {
        emit(context, "pcmpeqd xmm%d, xmm%d", mask, mask);
    }
    emit(context, "mov r8d, %d", (1 << width) - 1);
    emit(context, "mov r9d, r11d");
    emit_label(context, test);
    emit_activity_test(context, &loop, body, done, restore);
    emit_label(context, body);
    emit_step(context, &loop);
    emit(context, "dec r9d");
    emit(context, "jnz %s", test);
    if (loop.avx) {
        emit(context, "popcnt eax, r8d");
    } else {
        emit(context, "mov eax, r8d");
        emit(context, "and r8d, 1");
        emit(context, "shr eax, 1");
        emit(context, "add eax, r8d");
    }
    emit(context, "add %s, rax", total);
    if (loop.total->type == TYPE_INT) {
        emit(context, "mov rax, %s", total);
        emit_narrow(context, TYPE_INT);
        emit(context, "mov %s, rax", total);
    }
    emit_label(context, done);
    emit(context, "add %s, %d", index, width);
    emit(context, "dec r10");
    emit(context, "jnz %s", group);
    if (loop.avx) {
        emit(context, "vzeroupper");
    } else {
        emit_label(context, restore);
    }
    for (int i = FIRST_SAVED_DOUBLE_VECTOR; i < registers; i++) {
        emit(context, "movups xmm%d, %s", i, saved[i - FIRST_SAVED_DOUBLE_VECTOR].text + 6);
        release_temp(context);
        release_temp(context);
    }
    emit(context, "jmp %s", scalar);
    emit(context, "align 16, db 0");
    emit_label(context, lanes);
    emit(context, "dd 0, 1, 2, 3");
    emit_label(context, scalar);
    return true;
}

#include "codegen/codegen.h"

void emit_narrow(FunctionContext *context, ValueType type) {
    switch (type) {
        case TYPE_BOOL:
        case TYPE_CHAR:
        case TYPE_BYTE: emit(context, "movzx eax, al"); return;
        case TYPE_SBYTE: emit(context, "movsx rax, al"); return;
        case TYPE_SHORT: emit(context, "movsx rax, ax"); return;
        case TYPE_USHORT: emit(context, "movzx eax, ax"); return;
        case TYPE_INT: emit(context, "movsxd rax, eax"); return;
        case TYPE_UINT: emit(context, "mov eax, eax"); return;
        default: return;
    }
}

void emit_int_to_double(FunctionContext *context, ValueType source) {
    if (source != TYPE_ULONG) {
        emit(context, "cvtsi2sd xmm0, rax");
        return;
    }
    const char *large = new_label(context, "large");
    const char *done = new_label(context, "done");
    emit(context, "test rax, rax");
    emit(context, "js %s", large);
    emit(context, "cvtsi2sd xmm0, rax");
    emit(context, "jmp %s", done);
    emit_label(context, large);
    emit(context, "mov rcx, rax");
    emit(context, "shr rcx, 1");
    emit(context, "and eax, 1");
    emit(context, "or rcx, rax");
    emit(context, "cvtsi2sd xmm0, rcx");
    emit(context, "addsd xmm0, xmm0");
    emit_label(context, done);
}

void emit_double_to_int(FunctionContext *context, ValueType target) {
    if (target != TYPE_ULONG) {
        emit(context, "cvttsd2si rax, xmm0");
        emit_narrow(context, target);
        return;
    }
    const char *large = new_label(context, "large");
    const char *done = new_label(context, "done");
    emit(context, "movsd xmm1, qword [num@%d]", number_constant_index(context->program, 9223372036854775808.0));
    emit(context, "ucomisd xmm0, xmm1");
    emit(context, "jae %s", large);
    emit(context, "cvttsd2si rax, xmm0");
    emit(context, "jmp %s", done);
    emit_label(context, large);
    emit(context, "subsd xmm0, xmm1");
    emit(context, "cvttsd2si rax, xmm0");
    emit(context, "btc rax, 63");
    emit_label(context, done);
}

void emit_divide(FunctionContext *context, ValueType operation) {
    if (operation == TYPE_ULONG) {
        emit(context, "xor edx, edx");
        emit(context, "div rcx");
        return;
    }
    if (operation != TYPE_LONG) {
        emit(context, "cqo");
        emit(context, "idiv rcx");
        return;
    }
    const char *divide = new_label(context, "divide");
    const char *done = new_label(context, "divided");
    emit(context, "cmp rcx, -1");
    emit(context, "jne %s", divide);
    emit(context, "neg rax");
    emit(context, "xor edx, edx");
    emit(context, "jmp %s", done);
    emit_label(context, divide);
    emit(context, "cqo");
    emit(context, "idiv rcx");
    emit_label(context, done);
}

void emit_load_floating(FunctionContext *context, const Symbol *symbol, const char *target) {
    if (symbol->type == TYPE_FLOAT) {
        emit(context, "cvtss2sd %s, %s", target, sized_operand(symbol, "dword").text);
    } else {
        emit(context, "movsd %s, %s", target, memory_operand(symbol).text);
    }
}

void emit_store_floating(FunctionContext *context, const Symbol *symbol) {
    if (symbol->type == TYPE_FLOAT) {
        emit(context, "cvtsd2ss xmm0, xmm0");
        emit(context, "movss %s, xmm0", sized_operand(symbol, "dword").text);
    } else {
        emit(context, "movsd %s, xmm0", memory_operand(symbol).text);
    }
}

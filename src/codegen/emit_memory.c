#include "codegen/codegen.h"

static const char *const REGISTER_NAMES[2][4] = {
    {"al", "ax", "eax", "rax"},
    {"cl", "cx", "ecx", "rcx"},
};

static int size_index(int size) {
    return size == 1 ? 0 : size == 2 ? 1 : size == 4 ? 2 : 3;
}

const char *size_word(ValueType type) {
    static const char *const WORDS[] = {"byte", "word", "dword", "qword"};
    return WORDS[size_index(type_info(type)->size)];
}

void emit_load_from(FunctionContext *context, ValueType type, const char *place) {
    switch (type) {
        case TYPE_BOOL:
        case TYPE_CHAR:
        case TYPE_BYTE: emit(context, "movzx eax, byte [%s]", place); return;
        case TYPE_SBYTE: emit(context, "movsx rax, byte [%s]", place); return;
        case TYPE_SHORT: emit(context, "movsx rax, word [%s]", place); return;
        case TYPE_USHORT: emit(context, "movzx eax, word [%s]", place); return;
        case TYPE_INT: emit(context, "movsxd rax, dword [%s]", place); return;
        case TYPE_UINT: emit(context, "mov eax, dword [%s]", place); return;
        case TYPE_FLOAT: emit(context, "cvtss2sd xmm0, dword [%s]", place); return;
        case TYPE_DOUBLE: emit(context, "movsd xmm0, qword [%s]", place); return;
        default: emit(context, "mov rax, qword [%s]", place); return;
    }
}

void emit_store_to(FunctionContext *context, ValueType type, const char *place, bool from_rcx) {
    if (type == TYPE_FLOAT) {
        emit(context, "cvtsd2ss xmm0, xmm0");
        emit(context, "movss dword [%s], xmm0", place);
        return;
    }
    if (type == TYPE_DOUBLE) {
        emit(context, "movsd qword [%s], xmm0", place);
        return;
    }
    emit(context, "mov %s [%s], %s", size_word(type), place, REGISTER_NAMES[from_rcx][size_index(type_info(type)->size)]);
}

const char *sized_register(ValueType type, bool is_rcx) {
    return REGISTER_NAMES[is_rcx][size_index(type_info(type)->size)];
}

#include "codegen/codegen.h"

void emit_new(FunctionContext *context, Expression *expression) {
    int size = shape_size(expression->allocation.element);
    Expression *count = expression->allocation.count;
    if (count == NULL) {
        emit(context, "mov ecx, %d", size);
    } else {
        emit_int(context, count);
        if (size != 1) {
            emit(context, "imul rax, rax, %d", size);
        }
        emit(context, "mov rcx, rax");
    }
    emit(context, "call rz@allocate");
}

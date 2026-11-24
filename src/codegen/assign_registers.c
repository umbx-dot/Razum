#include "codegen/codegen.h"

#include "analysis/analysis.h"

static const char *const SAVED_REGISTERS[] = {"rbx", "rsi", "rdi", "r12", "r13", "r14", "r15"};
enum { SAVED_REGISTER_COUNT = sizeof SAVED_REGISTERS / sizeof SAVED_REGISTERS[0] };
static const char *const SAVED_VECTORS[] = {"xmm6", "xmm7", "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"};
enum { SAVED_VECTOR_COUNT = sizeof SAVED_VECTORS / sizeof SAVED_VECTORS[0] };

static bool fits_register(const Symbol *symbol) {
    ValueType type = symbol->type;
    bool is_scalar = is_integer(type) || type == TYPE_BOOL || type == TYPE_POINTER || type == TYPE_STRING || type == TYPE_FUNCTION ||
                     is_floating(type);
    return is_scalar && !symbol->address_taken && !symbol->is_reference && symbol->length_label == NULL;
}

static bool keep_tail_parameter(const Function *function, const Symbol *symbol) {
    ValueType result = function->return_type;
    return symbol->kind == SYMBOL_PARAMETER && function->parameters.count == 1 &&
           (symbol->type == TYPE_LONG || symbol->type == TYPE_ULONG) && (result == TYPE_LONG || result == TYPE_ULONG);
}

static void assign_integer(FunctionContext *context, Symbol *symbol, Symbol **assigned) {
    int slot = context->saved_count;
    if (slot < SAVED_REGISTER_COUNT) {
        context->saved_registers[context->saved_count++] = SAVED_REGISTERS[slot];
    } else {
        if (symbol->loop_uses == 0) {
            return;
        }
        slot = SAVED_REGISTER_COUNT - 1;
        int first = keep_tail_parameter(context->function, assigned[0]) ? 1 : 0;
        for (int index = slot - 1; index >= first; index--) {
            if (assigned[index]->loop_uses < assigned[slot]->loop_uses) {
                slot = index;
            }
        }
        if (symbol->loop_uses <= 3 * assigned[slot]->loop_uses) {
            return;
        }
        assigned[slot]->register_name = NULL;
    }
    assigned[slot] = symbol;
    symbol->register_name = SAVED_REGISTERS[slot];
}

static void assign(FunctionContext *context, Symbol *symbol, Symbol **integers) {
    if (!fits_register(symbol)) {
        return;
    }
    if (is_floating(symbol->type)) {
        if (context->saved_vector_count < SAVED_VECTOR_COUNT) {
            symbol->register_name = SAVED_VECTORS[context->saved_vector_count];
            context->saved_vectors[context->saved_vector_count++] = symbol->register_name;
        }
        return;
    }
    assign_integer(context, symbol, integers);
}

void assign_registers(FunctionContext *context) {
    Function *function = context->function;
    if (!context->program->optimize || contains_asm(function->body)) {
        return;
    }
    Symbol *integers[SAVED_REGISTER_COUNT] = {0};
    for (int index = 0; index < function->parameters.count; index++) {
        assign(context, LIST_AT(function->parameters, Parameter, index)->symbol, integers);
    }
    for (int index = 0; index < function->locals.count; index++) {
        assign(context, LIST_AT(function->locals, Symbol, index), integers);
    }
}

const char *reserve_loop_vector(FunctionContext *context, int index) {
    int slot = context->loop_vector_base + index;
    if (slot >= SAVED_VECTOR_COUNT) {
        return NULL;
    }
    context->saved_vectors[slot] = SAVED_VECTORS[slot];
    if (context->saved_vector_count <= slot) {
        context->saved_vector_count = slot + 1;
    }
    return SAVED_VECTORS[slot];
}

#include "codegen/codegen.h"

#include <string.h>

#include "common/arena.h"

int string_constant_index(ProgramContext *program, const char *bytes, size_t length) {
    for (int index = 0; index < program->strings.count; index++) {
        StringConstant *constant = LIST_AT(program->strings, StringConstant, index);
        if (constant->length == length && memcmp(constant->bytes, bytes, length) == 0) {
            return index + 1;
        }
    }
    StringConstant *constant = arena_alloc(sizeof *constant);
    constant->bytes = bytes;
    constant->length = length;
    list_push(&program->strings, constant);
    return program->strings.count;
}

int number_constant_index(ProgramContext *program, double value) {
    uint64_t bits = double_bits(value);
    for (int index = 0; index < program->numbers.count; index++) {
        if (*LIST_AT(program->numbers, uint64_t, index) == bits) {
            return index + 1;
        }
    }
    uint64_t *stored = arena_alloc(sizeof *stored);
    *stored = bits;
    list_push(&program->numbers, stored);
    return program->numbers.count;
}

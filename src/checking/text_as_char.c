#include "checking/checking.h"

#include "common/diagnostics.h"

static bool is_both_texts(const Expression *value) {
    return value->kind == EXPRESSION_CONDITIONAL && value->conditional.when_true->kind == EXPRESSION_STRING &&
           value->conditional.when_false->kind == EXPRESSION_STRING;
}

bool is_one_character_text(const Expression *value) {
    if (is_both_texts(value)) {
        return is_one_character_text(value->conditional.when_true) && is_one_character_text(value->conditional.when_false);
    }
    return value->kind == EXPRESSION_STRING && value->string.length == 1;
}

void text_as_char(Expression *value) {
    if (is_both_texts(value)) {
        text_as_char(value->conditional.when_true);
        text_as_char(value->conditional.when_false);
        value->type = TYPE_CHAR;
        value->shape = NULL;
        return;
    }
    if (value->kind != EXPRESSION_STRING) {
        return;
    }
    if (value->string.length != 1) {
        compile_error(value->line, value->column,
                      "a char holds one byte, but this text is %zu bytes; use a string, or one character such as \"a\" or \"\\n\"",
                      value->string.length);
    }
    int64_t byte = (unsigned char)value->string.bytes[0];
    value->kind = EXPRESSION_INT;
    value->int_literal.value = byte;
    value->int_literal.too_large = false;
    value->int_literal.literal_type = TYPE_CHAR;
    value->int_literal.from_constant = false;
    value->type = TYPE_CHAR;
    value->shape = NULL;
}

void store_text_as_char(const Shape *target, Expression *value) {
    if (target->kind == TYPE_CHAR && target->enumeration == NULL) {
        text_as_char(value);
    }
}

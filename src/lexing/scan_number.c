#include "lexing/lexing.h"

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "common/diagnostics.h"
#include "lexing/vocabulary.h"

static int skip_digits(SourceCursor *cursor, bool hexadecimal) {
    int count = 0;
    while (hexadecimal ? is_hex_digit(cursor_peek(cursor, 0)) : is_digit(cursor_peek(cursor, 0))) {
        cursor_advance(cursor, 1);
        count++;
    }
    return count;
}

static Token finish_int(SourceCursor *cursor, size_t start, int line, int column, int base) {
    Token token = token_at(cursor, TOKEN_INT, start, line, column);
    errno = 0;
    unsigned long long value = strtoull(base == 16 ? token.text + 2 : token.text, NULL, base);
    if (errno == ERANGE) {
        compile_error(line, column, "%s does not fit in 64 bits; the largest number is 18446744073709551615", token.text);
    }
    token.int_value = value;
    return token;
}

static Token finish_double(SourceCursor *cursor, size_t start, int line, int column) {
    Token token = token_at(cursor, TOKEN_DOUBLE, start, line, column);
    token.double_value = strtod(token.text, NULL);
    if (isinf(token.double_value)) {
        compile_error(line, column, "%s is too large for a double", token.text);
    }
    return token;
}

static void reject_letters(SourceCursor *cursor, int line, int column) {
    if (is_word_part(cursor_peek(cursor, 0))) {
        compile_error(line, column, "a number cannot be followed directly by '%c'", cursor_peek(cursor, 0));
    }
}

Token scan_number(SourceCursor *cursor) {
    int line = cursor->line;
    int column = cursor->column;
    size_t start = cursor->position;
    if (cursor_starts_with(cursor, "0x") || cursor_starts_with(cursor, "0X")) {
        cursor_advance(cursor, 2);
        if (skip_digits(cursor, true) == 0) {
            compile_error(line, column, "a hexadecimal number needs digits after 0x");
        }
        reject_letters(cursor, line, column);
        return finish_int(cursor, start, line, column, 16);
    }
    bool is_double = false;
    skip_digits(cursor, false);
    if (cursor_peek(cursor, 0) == '.') {
        is_double = true;
        cursor_advance(cursor, 1);
        skip_digits(cursor, false);
    }
    if (cursor_peek(cursor, 0) == 'e' || cursor_peek(cursor, 0) == 'E') {
        is_double = true;
        cursor_advance(cursor, 1);
        if (cursor_peek(cursor, 0) == '+' || cursor_peek(cursor, 0) == '-') {
            cursor_advance(cursor, 1);
        }
        if (skip_digits(cursor, false) == 0) {
            compile_error(line, column, "an exponent needs digits, as in 1e10");
        }
    }
    reject_letters(cursor, line, column);
    return is_double ? finish_double(cursor, start, line, column) : finish_int(cursor, start, line, column, 10);
}

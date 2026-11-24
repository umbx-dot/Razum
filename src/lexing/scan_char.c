#include "lexing/lexing.h"

#include "common/diagnostics.h"
#include "lexing/vocabulary.h"

Token scan_char(SourceCursor *cursor) {
    int line = cursor->line;
    int column = cursor->column;
    size_t start = cursor->position;
    char character = cursor_peek(cursor, 1);
    int value = (unsigned char)character;
    int width = 1;
    if (character == '\\') {
        value = escape_character(cursor_peek(cursor, 2));
        if (value < 0) {
            compile_error(line, column, "unknown escape \\%c; use \\n \\t \\r \\0 \\\\ \\\" or \\'", cursor_peek(cursor, 2));
        }
        width = 2;
    } else if (character == '\'' || character == '\n' || cursor_at_end(cursor)) {
        compile_error(line, column, "a char literal holds exactly one character, as in 'a'");
    }
    if (cursor_peek(cursor, 1 + width) != '\'') {
        compile_error(line, column, "a char literal holds exactly one character and ends with ', as in 'a'");
    }
    cursor_advance(cursor, (size_t)width + 2);
    Token token = token_at(cursor, TOKEN_CHAR, start, line, column);
    token.int_value = (uint64_t)value;
    return token;
}

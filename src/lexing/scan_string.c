#include "lexing/lexing.h"

#include <stdlib.h>

#include "common/arena.h"
#include "common/diagnostics.h"
#include "common/text_buffer.h"
#include "lexing/vocabulary.h"

Token scan_string(SourceCursor *cursor) {
    int line = cursor->line;
    int column = cursor->column;
    size_t start = cursor->position;
    TextBuffer value = {0};
    cursor_advance(cursor, 1);
    while (cursor_peek(cursor, 0) != '"') {
        if (cursor_at_end(cursor) || cursor_peek(cursor, 0) == '\n') {
            compile_error(line, column, "string is never closed with \"");
        }
        char character = cursor_peek(cursor, 0);
        if (character == '\\') {
            int escaped = escape_character(cursor_peek(cursor, 1));
            if (escaped < 0) {
                compile_error(cursor->line, cursor->column, "unknown escape \\%c; use \\n \\t \\r \\0 \\\\ \\\" or \\'", cursor_peek(cursor, 1));
            }
            character = (char)escaped;
            cursor_advance(cursor, 2);
        } else {
            cursor_advance(cursor, 1);
        }
        text_append_bytes(&value, &character, 1);
    }
    cursor_advance(cursor, 1);
    Token token = token_at(cursor, TOKEN_STRING, start, line, column);
    token.string_length = value.length;
    token.string_value = arena_copy_text(value.length > 0 ? value.data : "", value.length);
    free(value.data);
    return token;
}

#include "lexing/lexing.h"

#include <string.h>

#include "common/diagnostics.h"
#include "lexing/vocabulary.h"

Token scan_operator(SourceCursor *cursor) {
    int line = cursor->line;
    int column = cursor->column;
    size_t start = cursor->position;
    for (int index = 0; index < OPERATOR_COUNT; index++) {
        if (cursor_starts_with(cursor, OPERATORS[index])) {
            cursor_advance(cursor, strlen(OPERATORS[index]));
            return token_at(cursor, TOKEN_OPERATOR, start, line, column);
        }
    }
    char character = cursor_peek(cursor, 0);
    if ((unsigned char)character < 0x20 || (unsigned char)character >= 0x7F) {
        compile_error(line, column, "unexpected byte 0x%02X", (unsigned char)character);
    }
    compile_error(line, column, "unexpected character '%c'", character);
}

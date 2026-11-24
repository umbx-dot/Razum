#include "lexing/lexing.h"

#include "common/diagnostics.h"

static void skip_block_comment(SourceCursor *cursor) {
    int line = cursor->line;
    int column = cursor->column;
    cursor_advance(cursor, 2);
    while (!cursor_starts_with(cursor, "*/")) {
        if (cursor_at_end(cursor)) {
            compile_error(line, column, "comment is never closed with */");
        }
        cursor_advance(cursor, 1);
    }
    cursor_advance(cursor, 2);
}

void skip_trivia(SourceCursor *cursor) {
    while (!cursor_at_end(cursor)) {
        char character = cursor_peek(cursor, 0);
        if (character == ' ' || character == '\t' || character == '\r' || character == '\n') {
            cursor_advance(cursor, 1);
        } else if (cursor_starts_with(cursor, "//")) {
            while (!cursor_at_end(cursor) && cursor_peek(cursor, 0) != '\n') {
                cursor_advance(cursor, 1);
            }
        } else if (cursor_starts_with(cursor, "/*")) {
            skip_block_comment(cursor);
        } else {
            return;
        }
    }
}

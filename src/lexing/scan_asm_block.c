#include "lexing/lexing.h"

#include "common/arena.h"
#include "common/diagnostics.h"

static void skip_quoted(SourceCursor *cursor, int line, int column) {
    char quote = cursor_peek(cursor, 0);
    cursor_advance(cursor, 1);
    while (cursor_peek(cursor, 0) != quote) {
        if (cursor_at_end(cursor) || cursor_peek(cursor, 0) == '\n') {
            compile_error(line, column, "a quoted text inside this asm block is never closed");
        }
        cursor_advance(cursor, cursor_peek(cursor, 0) == '\\' && quote == '`' ? 2 : 1);
    }
    cursor_advance(cursor, 1);
}

Token scan_asm_block(SourceCursor *cursor, int line, int column) {
    while (cursor_peek(cursor, 0) == ' ' || cursor_peek(cursor, 0) == '\t' || cursor_peek(cursor, 0) == '\r' || cursor_peek(cursor, 0) == '\n') {
        cursor_advance(cursor, 1);
    }
    if (cursor_peek(cursor, 0) != '{') {
        compile_error(line, column, "asm must be followed by a block: asm { instructions }");
    }
    cursor_advance(cursor, 1);
    size_t start = cursor->position;
    int depth = 1;
    for (;;) {
        if (cursor_at_end(cursor)) {
            compile_error(line, column, "the asm block opened here is never closed with }");
        }
        char character = cursor_peek(cursor, 0);
        if (character == ';') {
            while (!cursor_at_end(cursor) && cursor_peek(cursor, 0) != '\n') {
                cursor_advance(cursor, 1);
            }
        } else if (character == '\'' || character == '"' || character == '`') {
            skip_quoted(cursor, cursor->line, cursor->column);
        } else {
            if (character == '{') {
                depth++;
            } else if (character == '}' && --depth == 0) {
                break;
            }
            cursor_advance(cursor, 1);
        }
    }
    Token token = {0};
    token.kind = TOKEN_ASM;
    token.text = arena_copy_text(cursor->text + start, cursor->position - start);
    token.line = line;
    token.column = column;
    cursor_advance(cursor, 1);
    return token;
}

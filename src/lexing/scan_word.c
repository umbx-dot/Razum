#include "lexing/lexing.h"

#include <string.h>

#include "lexing/vocabulary.h"

Token scan_word(SourceCursor *cursor) {
    int line = cursor->line;
    int column = cursor->column;
    size_t start = cursor->position;
    while (is_word_part(cursor_peek(cursor, 0))) {
        cursor_advance(cursor, 1);
    }
    Token token = token_at(cursor, TOKEN_NAME, start, line, column);
    if (strcmp(token.text, "asm") == 0) {
        return scan_asm_block(cursor, line, column);
    }
    if (is_keyword(token.text)) {
        token.kind = TOKEN_KEYWORD;
    }
    return token;
}

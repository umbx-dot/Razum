#include "lexing/lexing.h"

#include "common/arena.h"

Token token_at(const SourceCursor *cursor, TokenKind kind, size_t start, int line, int column) {
    Token token = {0};
    token.kind = kind;
    token.text = arena_copy_text(cursor->text + start, cursor->position - start);
    token.line = line;
    token.column = column;
    return token;
}

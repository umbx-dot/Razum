#include "lexing/lexing.h"

#include <string.h>

#include "common/list.h"
#include "common/arena.h"
#include "lexing/vocabulary.h"

static Token scan_token(SourceCursor *cursor) {
    char character = cursor_peek(cursor, 0);
    if (is_word_start(character)) {
        return scan_word(cursor);
    }
    if (is_digit(character) || (character == '.' && is_digit(cursor_peek(cursor, 1)))) {
        return scan_number(cursor);
    }
    if (character == '"') {
        return scan_string(cursor);
    }
    if (character == '\'') {
        return scan_char(cursor);
    }
    return scan_operator(cursor);
}

TokenArray tokenize(const char *text, size_t length) {
    SourceCursor cursor = {text, length, 0, 1, 1};
    static const char BYTE_ORDER_MARK[] = "\xEF\xBB\xBF";
    if (length >= 3 && memcmp(text, BYTE_ORDER_MARK, 3) == 0) {
        cursor.position = 3;
    }
    PointerList tokens = {0};
    for (;;) {
        skip_trivia(&cursor);
        Token *token = arena_alloc(sizeof *token);
        if (cursor_at_end(&cursor)) {
            *token = token_at(&cursor, TOKEN_END, cursor.position, cursor.line, cursor.column);
            list_push(&tokens, token);
            break;
        }
        *token = scan_token(&cursor);
        list_push(&tokens, token);
    }
    TokenArray result = {arena_alloc((size_t)tokens.count * sizeof(Token)), tokens.count};
    for (int index = 0; index < tokens.count; index++) {
        result.items[index] = *LIST_AT(tokens, Token, index);
    }
    return result;
}

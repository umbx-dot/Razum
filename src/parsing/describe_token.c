#include "parsing/token_stream.h"

#include <string.h>

#include "common/arena.h"

const char *describe_token(const Token *token) {
    if (token->kind == TOKEN_END) {
        return "the end of the file";
    }
    size_t length = strlen(token->text);
    char *description = arena_alloc(length + 3);
    description[0] = '\'';
    memcpy(description + 1, token->text, length);
    description[length + 1] = '\'';
    description[length + 2] = '\0';
    return description;
}

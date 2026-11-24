#include "parsing/token_stream.h"

#include <string.h>

#include "common/diagnostics.h"
#include "lexing/vocabulary.h"

const Token *stream_current(const TokenStream *stream) {
    return &stream->tokens.items[stream->index];
}

const Token *stream_peek(const TokenStream *stream, int offset) {
    int index = stream->index + offset;
    return &stream->tokens.items[index < stream->tokens.count ? index : stream->tokens.count - 1];
}

const Token *stream_advance(TokenStream *stream) {
    const Token *token = stream_current(stream);
    if (token->kind != TOKEN_END) {
        stream->index++;
    }
    return token;
}

bool stream_is_operator(const TokenStream *stream, const char *text, int offset) {
    const Token *token = stream_peek(stream, offset);
    return token->kind == TOKEN_OPERATOR && strcmp(token->text, text) == 0;
}

bool stream_is_keyword(const TokenStream *stream, const char *text) {
    const Token *token = stream_current(stream);
    return token->kind == TOKEN_KEYWORD && strcmp(token->text, text) == 0;
}

bool stream_is_type(const TokenStream *stream, int offset) {
    const Token *token = stream_peek(stream, offset);
    return token->kind == TOKEN_KEYWORD && is_type_name(token->text);
}

bool stream_accept_operator(TokenStream *stream, const char *text) {
    if (!stream_is_operator(stream, text, 0)) {
        return false;
    }
    stream_advance(stream);
    return true;
}

bool stream_accept_keyword(TokenStream *stream, const char *text) {
    if (!stream_is_keyword(stream, text)) {
        return false;
    }
    stream_advance(stream);
    return true;
}

const Token *stream_expect_operator(TokenStream *stream, const char *text) {
    const Token *token = stream_current(stream);
    if (!stream_is_operator(stream, text, 0)) {
        bool is_step = token->kind == TOKEN_OPERATOR && (strcmp(token->text, "++") == 0 || strcmp(token->text, "--") == 0);
        if (is_step) {
            compile_error(token->line, token->column, "'%s' is a statement of its own in razum, not part of an expression; write it on its own line, as in x%s;",
                          token->text, token->text);
        }
        compile_error(token->line, token->column, "expected '%s', found %s", text, describe_token(token));
    }
    return stream_advance(stream);
}

const Token *stream_expect_name(TokenStream *stream, const char *what) {
    const Token *token = stream_current(stream);
    if (token->kind != TOKEN_NAME) {
        compile_error(token->line, token->column, "expected %s, found %s", what, describe_token(token));
    }
    return stream_advance(stream);
}

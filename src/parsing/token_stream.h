#pragma once

#include <stdbool.h>

#include "lexing/token.h"

typedef struct {
    TokenArray tokens;
    int index;
} TokenStream;

const Token *stream_current(const TokenStream *stream);
const Token *stream_peek(const TokenStream *stream, int offset);
const Token *stream_advance(TokenStream *stream);
bool stream_is_operator(const TokenStream *stream, const char *text, int offset);
bool stream_is_keyword(const TokenStream *stream, const char *text);
bool stream_is_type(const TokenStream *stream, int offset);
bool stream_accept_operator(TokenStream *stream, const char *text);
bool stream_accept_keyword(TokenStream *stream, const char *text);
const Token *stream_expect_operator(TokenStream *stream, const char *text);
const Token *stream_expect_name(TokenStream *stream, const char *what);
const char *describe_token(const Token *token);

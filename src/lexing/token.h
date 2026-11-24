#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum {
    TOKEN_NAME,
    TOKEN_KEYWORD,
    TOKEN_INT,
    TOKEN_DOUBLE,
    TOKEN_STRING,
    TOKEN_CHAR,
    TOKEN_OPERATOR,
    TOKEN_ASM,
    TOKEN_END,
} TokenKind;

typedef struct {
    TokenKind kind;
    const char *text;
    int line;
    int column;
    uint64_t int_value;
    double double_value;
    const char *string_value;
    size_t string_length;
} Token;

typedef struct {
    Token *items;
    int count;
} TokenArray;

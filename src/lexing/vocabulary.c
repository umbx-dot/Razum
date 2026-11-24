#include "lexing/vocabulary.h"

#include <string.h>

static const char *const KEYWORDS[] = {
    "import", "asm", "if", "else", "while", "for", "return", "break", "continue", "true", "false", "struct", "switch", "case", "default", "do", "extern", "export", "null", "new", "sizeof", "fn", "const", "enum",
};

static const char *const TYPE_NAMES[] = {
    "void", "bool", "char", "byte", "sbyte", "short", "ushort", "int", "uint",
    "long", "ulong", "float", "double", "string",
};

const char *const OPERATORS[] = {
    "<<=", ">>=",
    "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "->", "==", "!=", "<=", ">=", "&&", "||", "<<", ">>",
    "+", "-", "*", "/", "%", "<", ">", "=", "!", "&", "|", "^", "~", "?", ":", "(", ")", "{", "}", "[", "]", ",", ";", ".",
};

const int OPERATOR_COUNT = (int)(sizeof OPERATORS / sizeof OPERATORS[0]);

static bool in_list(const char *word, const char *const *list, size_t count) {
    for (size_t index = 0; index < count; index++) {
        if (strcmp(word, list[index]) == 0) {
            return true;
        }
    }
    return false;
}

bool is_keyword(const char *word) {
    return in_list(word, KEYWORDS, sizeof KEYWORDS / sizeof KEYWORDS[0]) || is_type_name(word);
}

bool is_type_name(const char *word) {
    return in_list(word, TYPE_NAMES, sizeof TYPE_NAMES / sizeof TYPE_NAMES[0]);
}

int escape_character(char letter) {
    switch (letter) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case '0': return '\0';
        case '\\': return '\\';
        case '"': return '"';
        case '\'': return '\'';
        default: return -1;
    }
}

bool is_digit(char character) {
    return character >= '0' && character <= '9';
}

bool is_hex_digit(char character) {
    return is_digit(character) || (character >= 'a' && character <= 'f') || (character >= 'A' && character <= 'F');
}

bool is_word_start(char character) {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_';
}

bool is_word_part(char character) {
    return is_word_start(character) || is_digit(character);
}

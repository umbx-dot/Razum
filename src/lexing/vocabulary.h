#pragma once

#include <stdbool.h>

extern const char *const OPERATORS[];
extern const int OPERATOR_COUNT;

bool is_keyword(const char *word);
bool is_type_name(const char *word);
int escape_character(char letter);
bool is_digit(char character);
bool is_hex_digit(char character);
bool is_word_start(char character);
bool is_word_part(char character);

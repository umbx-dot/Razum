#pragma once

#include "lexing/source_cursor.h"
#include "lexing/token.h"

TokenArray tokenize(const char *text, size_t length);

void skip_trivia(SourceCursor *cursor);
Token scan_word(SourceCursor *cursor);
Token scan_number(SourceCursor *cursor);
Token scan_string(SourceCursor *cursor);
Token scan_char(SourceCursor *cursor);
Token scan_operator(SourceCursor *cursor);
Token scan_asm_block(SourceCursor *cursor, int line, int column);
Token token_at(const SourceCursor *cursor, TokenKind kind, size_t start, int line, int column);

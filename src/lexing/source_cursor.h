#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char *text;
    size_t length;
    size_t position;
    int line;
    int column;
} SourceCursor;

bool cursor_at_end(const SourceCursor *cursor);
char cursor_peek(const SourceCursor *cursor, size_t offset);
bool cursor_starts_with(const SourceCursor *cursor, const char *prefix);
void cursor_advance(SourceCursor *cursor, size_t count);

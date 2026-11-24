#include "lexing/source_cursor.h"

#include <string.h>

bool cursor_at_end(const SourceCursor *cursor) {
    return cursor->position >= cursor->length;
}

char cursor_peek(const SourceCursor *cursor, size_t offset) {
    size_t index = cursor->position + offset;
    return index < cursor->length ? cursor->text[index] : '\0';
}

bool cursor_starts_with(const SourceCursor *cursor, const char *prefix) {
    size_t length = strlen(prefix);
    return cursor->position + length <= cursor->length && memcmp(cursor->text + cursor->position, prefix, length) == 0;
}

void cursor_advance(SourceCursor *cursor, size_t count) {
    for (size_t step = 0; step < count && cursor->position < cursor->length; step++) {
        if (cursor->text[cursor->position] == '\n') {
            cursor->line++;
            cursor->column = 1;
        } else {
            cursor->column++;
        }
        cursor->position++;
    }
}

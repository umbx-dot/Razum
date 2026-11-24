#include "linking/linking.h"

#include <string.h>

#include "common/arena.h"

static bool is_label_character(char character) {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9') || character == '_' || character == '@';
}

static bool contains(const PointerList *names, const char *name, size_t length) {
    for (int index = 0; index < names->count; index++) {
        const char *existing = LIST_AT(*names, const char, index);
        if (strlen(existing) == length && memcmp(existing, name, length) == 0) {
            return true;
        }
    }
    return false;
}

void collect_references(const char *text, const char *prefix, PointerList *names) {
    size_t prefix_length = strlen(prefix);
    for (const char *cursor = strstr(text, prefix); cursor != NULL; cursor = strstr(cursor + 1, prefix)) {
        if (cursor > text && is_label_character(cursor[-1])) {
            continue;
        }
        size_t length = prefix_length;
        while (is_label_character(cursor[length])) {
            length++;
        }
        if (length > prefix_length && !contains(names, cursor, length)) {
            list_push(names, arena_copy_text(cursor, length));
        }
    }
}

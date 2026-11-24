#include "common/arena.h"

#include <string.h>

char *arena_copy_text(const char *text, size_t length) {
    char *copy = arena_alloc(length + 1);
    memcpy(copy, text, length);
    copy[length] = '\0';
    return copy;
}

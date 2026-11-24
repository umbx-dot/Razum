#pragma once

#include <stddef.h>

void *arena_alloc(size_t size);
char *arena_copy_text(const char *text, size_t length);

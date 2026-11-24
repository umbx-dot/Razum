#pragma once

#include <stdbool.h>
#include <stddef.h>

char *read_file(const char *path, size_t *length);
bool write_file(const char *path, const char *data, size_t length);

#pragma once

#include <stdbool.h>

#include "common/list.h"

typedef enum {
    PATH_MISSING,
    PATH_FILE,
    PATH_DIRECTORY,
} PathKind;

PathKind path_kind(const char *path);
bool same_file(const char *left, const char *right);
void list_files(const char *directory, const char *extension, bool recursive, PointerList *paths);
bool make_executable(const char *path);

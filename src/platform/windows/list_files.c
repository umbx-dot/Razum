#include "platform/filesystem.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "common/arena.h"

static bool has_extension(const char *name, const char *extension) {
    size_t name_length = strlen(name);
    size_t extension_length = strlen(extension);
    return name_length > extension_length && _stricmp(name + name_length - extension_length, extension) == 0;
}

static int compare_paths(const void *left, const void *right) {
    return strcmp(*(const char *const *)left, *(const char *const *)right);
}

static void collect(const char *directory, const char *extension, bool recursive, PointerList *paths) {
    char pattern[MAX_PATH];
    snprintf(pattern, sizeof pattern, "%s/*", directory);
    WIN32_FIND_DATAA entry;
    HANDLE search = FindFirstFileA(pattern, &entry);
    if (search == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        if (strcmp(entry.cFileName, ".") == 0 || strcmp(entry.cFileName, "..") == 0) {
            continue;
        }
        size_t size = strlen(directory) + strlen(entry.cFileName) + 2;
        char *path = arena_alloc(size);
        snprintf(path, size, "%s/%s", directory, entry.cFileName);
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive) {
                collect(path, extension, recursive, paths);
            }
        } else if (has_extension(entry.cFileName, extension)) {
            list_push(paths, path);
        }
    } while (FindNextFileA(search, &entry));
    FindClose(search);
}

void list_files(const char *directory, const char *extension, bool recursive, PointerList *paths) {
    int first = paths->count;
    collect(directory, extension, recursive, paths);
    qsort(paths->items + first, (size_t)(paths->count - first), sizeof paths->items[0], compare_paths);
}

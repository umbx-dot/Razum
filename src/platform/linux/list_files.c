#include "platform/filesystem.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "common/arena.h"

static bool has_extension(const char *name, const char *extension) {
    size_t name_length = strlen(name);
    size_t extension_length = strlen(extension);
    return name_length > extension_length && strcasecmp(name + name_length - extension_length, extension) == 0;
}

static int compare_paths(const void *left, const void *right) {
    return strcmp(*(const char *const *)left, *(const char *const *)right);
}

static void collect(const char *directory, const char *extension, bool recursive, PointerList *paths) {
    DIR *folder = opendir(directory);
    if (folder == NULL) {
        return;
    }
    for (struct dirent *entry = readdir(folder); entry != NULL; entry = readdir(folder)) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        size_t size = strlen(directory) + strlen(entry->d_name) + 2;
        char *path = arena_alloc(size);
        snprintf(path, size, "%s/%s", directory, entry->d_name);
        PathKind kind = path_kind(path);
        if (kind == PATH_DIRECTORY) {
            if (recursive) {
                collect(path, extension, recursive, paths);
            }
        } else if (kind == PATH_FILE && has_extension(entry->d_name, extension)) {
            list_push(paths, path);
        }
    }
    closedir(folder);
}

void list_files(const char *directory, const char *extension, bool recursive, PointerList *paths) {
    int first = paths->count;
    collect(directory, extension, recursive, paths);
    qsort(paths->items + first, (size_t)(paths->count - first), sizeof paths->items[0], compare_paths);
}

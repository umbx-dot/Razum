#define _XOPEN_SOURCE 700
#include "project.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int compare(const void *left, const void *right) {
    return strcmp(*(const char *const *)left, *(const char *const *)right);
}

static bool collect(const char *folder, const char *excluded, const char *extension, PointerList *paths) {
    DIR *dir = opendir(folder);
    if (dir == NULL) return false;
    bool ok = true;
    for (struct dirent *entry = readdir(dir); entry != NULL; entry = readdir(dir)) {
        const char *name = entry->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || strcmp(name, ".git") == 0) continue;
        const char *path = rz_join(folder, name);
        if (excluded != NULL && strcmp(path, excluded) == 0) continue;
        struct stat info;
        if (lstat(path, &info) != 0 || (!S_ISDIR(info.st_mode) && !S_ISREG(info.st_mode))) {
            ok = false;
            break;
        }
        if (S_ISDIR(info.st_mode)) {
            if (!collect(path, excluded, extension, paths)) {
                ok = false;
                break;
            }
        } else {
            size_t length = strlen(name), suffix = strlen(extension);
            if (length > suffix && strcmp(name + length - suffix, extension) == 0) {
                rz_add_argument(paths, path);
            }
        }
    }
    closedir(dir);
    return ok;
}

bool rz_list_inputs(const char *folder, const char *excluded, const char *extension, PointerList *paths) {
    bool ok = collect(folder, excluded, extension, paths);
    if (paths->count > 1) {
        qsort(paths->items, (size_t)paths->count, sizeof paths->items[0], compare);
    }
    return ok;
}

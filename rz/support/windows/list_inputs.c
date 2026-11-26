#include "project.h"

#include <stdlib.h>
#include <string.h>
#include <windows.h>

static int compare(const void *left, const void *right) {
    return strcmp(*(const char *const *)left, *(const char *const *)right);
}

static bool collect(const char *folder, const char *excluded, const char *extension, PointerList *paths) {
    WIN32_FIND_DATAA entry;
    HANDLE search = FindFirstFileA(rz_join(folder, "*"), &entry);
    if (search == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_FILE_NOT_FOUND;
    bool ok = true;
    do {
        const char *name = entry.cFileName;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || _stricmp(name, ".git") == 0) continue;
        const char *path = rz_join(folder, name);
        if (excluded != NULL && _stricmp(path, excluded) == 0) continue;
        if ((entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
            ok = false;
            break;
        }
        if ((entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            if (!collect(path, excluded, extension, paths)) {
                ok = false;
                break;
            }
        } else {
            size_t length = strlen(name), suffix = strlen(extension);
            if (length > suffix && _stricmp(name + length - suffix, extension) == 0) {
                rz_add_argument(paths, path);
            }
        }
    } while (FindNextFileA(search, &entry));
    FindClose(search);
    return ok;
}

bool rz_list_inputs(const char *folder, const char *excluded, const char *extension, PointerList *paths) {
    bool ok = collect(folder, excluded, extension, paths);
    if (paths->count > 1) {
        qsort(paths->items, (size_t)paths->count, sizeof paths->items[0], compare);
    }
    return ok;
}

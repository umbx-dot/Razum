#include "project.h"

#include <string.h>
#include <windows.h>

#include "common/arena.h"
#include "platform/filesystem.h"

const char *rz_absolute(const char *path) {
    char target[4096];
    DWORD count = GetFullPathNameA(path, (DWORD)sizeof target, target, NULL);
    if (count == 0 || count >= sizeof target) {
        rz_error("cannot resolve %s", path);
    }
    return arena_copy_text(target, count);
}

const char *rz_sibling(const char *name) {
    char path[4096];
    DWORD count = GetModuleFileNameA(NULL, path, (DWORD)sizeof path);
    if (count == 0 || count >= sizeof path) {
        return NULL;
    }
    return rz_join(rz_parent(path), name);
}

bool rz_make_directory(const char *path) {
    return CreateDirectoryA(path, NULL) != 0;
}

bool rz_replace(const char *from, const char *to) {
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING) != 0;
}

bool rz_remove_directory(const char *path) {
    return RemoveDirectoryA(path) != 0;
}

uintptr_t rz_lock_file(const char *path) {
    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    return file == INVALID_HANDLE_VALUE ? 0 : (uintptr_t)file;
}

void rz_unlock_file(uintptr_t lock) {
    if (lock != 0) CloseHandle((HANDLE)lock);
}

bool rz_no_links(const char *path, bool recursive) {
    DWORD attributes = GetFileAttributesA(path);
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
        return false;
    }
    if (!recursive || (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        return true;
    }
    WIN32_FIND_DATAA item;
    HANDLE folder = FindFirstFileA(rz_join(path, "*"), &item);
    if (folder == INVALID_HANDLE_VALUE) {
        return GetLastError() == ERROR_FILE_NOT_FOUND;
    }
    bool okay = true;
    do {
        if (strcmp(item.cFileName, ".") != 0 && strcmp(item.cFileName, "..") != 0 &&
            !rz_no_links(rz_join(path, item.cFileName), true)) {
            okay = false;
            break;
        }
    } while (FindNextFileA(folder, &item));
    FindClose(folder);
    return okay;
}

double rz_milliseconds(void) {
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&frequency);
    return (double)counter.QuadPart * 1000 / (double)frequency.QuadPart;
}

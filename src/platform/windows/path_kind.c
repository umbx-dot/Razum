#include "platform/filesystem.h"

#include <windows.h>

PathKind path_kind(const char *path) {
    DWORD attributes = GetFileAttributesA(path);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return PATH_MISSING;
    }
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) ? PATH_DIRECTORY : PATH_FILE;
}

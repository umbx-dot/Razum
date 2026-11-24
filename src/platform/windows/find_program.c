#include "platform/process.h"

#include <windows.h>

bool find_program(const char *name, char *path, size_t size) {
    DWORD length = SearchPathA(NULL, name, ".exe", (DWORD)size, path, NULL);
    return length > 0 && length < size;
}

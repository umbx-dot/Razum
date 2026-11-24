#include "setup.h"

#include <windows.h>

bool extract_resource(int id, const wchar_t *path) {
    HRSRC resource = FindResourceW(NULL, MAKEINTRESOURCEW(id), MAKEINTRESOURCEW(10));
    HGLOBAL loaded = resource != NULL ? LoadResource(NULL, resource) : NULL;
    const void *data = loaded != NULL ? LockResource(loaded) : NULL;
    if (data == NULL) {
        return false;
    }
    DWORD size = SizeofResource(NULL, resource);
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD written = 0;
    bool complete = WriteFile(file, data, size, &written, NULL) && written == size;
    return CloseHandle(file) && complete;
}

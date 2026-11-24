#include "platform/filesystem.h"

#include <windows.h>

static bool file_identity(const char *path, BY_HANDLE_FILE_INFORMATION *identity) {
    HANDLE file = CreateFileA(path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    BOOL success = GetFileInformationByHandle(file, identity);
    CloseHandle(file);
    return success != 0;
}

bool same_file(const char *left, const char *right) {
    BY_HANDLE_FILE_INFORMATION first;
    BY_HANDLE_FILE_INFORMATION second;
    return file_identity(right, &second) && file_identity(left, &first) &&
           first.dwVolumeSerialNumber == second.dwVolumeSerialNumber &&
           first.nFileIndexHigh == second.nFileIndexHigh && first.nFileIndexLow == second.nFileIndexLow;
}

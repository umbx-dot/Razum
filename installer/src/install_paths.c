#include "setup.h"

#include <windows.h>

bool install_paths(InstallPaths *paths) {
    wchar_t local[PATH_SIZE];
    DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", local, PATH_SIZE);
    if (length == 0 || length >= PATH_SIZE) {
        return false;
    }
    int root = swprintf(paths->root, PATH_SIZE, L"%ls\\Programs\\razum", local);
    int bin = swprintf(paths->bin, PATH_SIZE, L"%ls\\bin", paths->root);
    int cmake = swprintf(paths->cmake, PATH_SIZE, L"%ls\\cmake", paths->root);
    return root > 0 && bin > 0 && cmake > 0;
}

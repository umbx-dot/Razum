#include "setup.h"

#include <windows.h>

#define PACKAGE_KEY L"Software\\Kitware\\CMake\\Packages\\Razum"

bool register_cmake_package(const InstallPaths *paths) {
    return set_user_string(PACKAGE_KEY, L"razum", paths->cmake);
}

void unregister_cmake_package(void) {
    RegDeleteTreeW(HKEY_CURRENT_USER, PACKAGE_KEY);
}

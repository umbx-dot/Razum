#include "setup.h"

#include <windows.h>

#define UNINSTALL_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\razum"

static bool set_number(const wchar_t *name, DWORD value) {
    return RegSetKeyValueW(HKEY_CURRENT_USER, UNINSTALL_KEY, name, REG_DWORD, &value, sizeof value) == ERROR_SUCCESS;
}

bool register_uninstall_entry(const InstallPaths *paths) {
    wchar_t icon[PATH_SIZE];
    wchar_t command[2 * PATH_SIZE];
    swprintf(icon, PATH_SIZE, L"%ls\\rz.ico", paths->root);
    swprintf(command, 2 * PATH_SIZE, L"\"%ls\\uninstall.exe\" --uninstall", paths->root);
    return set_user_string(UNINSTALL_KEY, L"DisplayName", L"razum") &&
           set_user_string(UNINSTALL_KEY, L"DisplayVersion", RAZUM_VERSION) &&
           set_user_string(UNINSTALL_KEY, L"Publisher", L"idom") &&
           set_user_string(UNINSTALL_KEY, L"DisplayIcon", icon) &&
           set_user_string(UNINSTALL_KEY, L"InstallLocation", paths->root) &&
           set_user_string(UNINSTALL_KEY, L"UninstallString", command) &&
           set_number(L"NoModify", 1) && set_number(L"NoRepair", 1);
}

void unregister_uninstall_entry(void) {
    RegDeleteTreeW(HKEY_CURRENT_USER, UNINSTALL_KEY);
}

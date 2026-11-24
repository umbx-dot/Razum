#include "setup.h"

#include <windows.h>

enum { LIST_SIZE = 32767 };

static wchar_t current[LIST_SIZE];
static wchar_t updated[LIST_SIZE];

static bool read_user_path(void) {
    DWORD size = sizeof current;
    LSTATUS status = RegGetValueW(HKEY_CURRENT_USER, L"Environment", L"Path",
                                  RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, NULL, current, &size);
    if (status == ERROR_FILE_NOT_FOUND) {
        current[0] = L'\0';
        return true;
    }
    return status == ERROR_SUCCESS;
}

static bool write_user_path(void) {
    DWORD size = (DWORD)((wcslen(updated) + 1) * sizeof(wchar_t));
    if (RegSetKeyValueW(HKEY_CURRENT_USER, L"Environment", L"Path", REG_EXPAND_SZ, updated, size) != ERROR_SUCCESS) {
        return false;
    }
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
    return true;
}

bool add_to_user_path(const wchar_t *directory) {
    if (!read_user_path()) {
        return false;
    }
    if (path_list_contains(current, directory)) {
        return true;
    }
    return path_list_add(current, directory, updated, LIST_SIZE) && write_user_path();
}

bool remove_from_user_path(const wchar_t *directory) {
    if (!read_user_path()) {
        return false;
    }
    if (!path_list_contains(current, directory)) {
        return true;
    }
    return path_list_remove(current, directory, updated, LIST_SIZE) && write_user_path();
}

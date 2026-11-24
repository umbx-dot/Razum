#include "setup.h"

#include <windows.h>

bool set_user_string(const wchar_t *key, const wchar_t *name, const wchar_t *value) {
    DWORD size = (DWORD)((wcslen(value) + 1) * sizeof(wchar_t));
    return RegSetKeyValueW(HKEY_CURRENT_USER, key, name, REG_SZ, value, size) == ERROR_SUCCESS;
}

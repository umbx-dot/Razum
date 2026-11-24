#include "setup.h"

#include <windows.h>

static bool is_file(const wchar_t *path) {
    DWORD attributes = GetFileAttributesW(path);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static bool executable_beside(VsCode *vscode) {
    wcscpy(vscode->executable, vscode->command);
    for (int level = 0; level < 2; level++) {
        wchar_t *slash = wcsrchr(vscode->executable, L'\\');
        if (slash == NULL) {
            return false;
        }
        *slash = L'\0';
    }
    static const wchar_t NAME[] = L"\\Code.exe";
    if (wcslen(vscode->executable) + wcslen(NAME) >= PATH_SIZE) {
        return false;
    }
    wcscat(vscode->executable, NAME);
    return is_file(vscode->executable);
}

bool find_vscode(VsCode *vscode) {
    DWORD found = SearchPathW(NULL, L"code.cmd", NULL, PATH_SIZE, vscode->command, NULL);
    if (found > 0 && found < PATH_SIZE && executable_beside(vscode)) {
        return true;
    }
    static const wchar_t *const CANDIDATES[] = {
        L"%LOCALAPPDATA%\\Programs\\Microsoft VS Code\\bin\\code.cmd",
        L"%ProgramFiles%\\Microsoft VS Code\\bin\\code.cmd",
    };
    for (size_t index = 0; index < sizeof CANDIDATES / sizeof CANDIDATES[0]; index++) {
        DWORD length = ExpandEnvironmentStringsW(CANDIDATES[index], vscode->command, PATH_SIZE);
        if (length > 0 && length <= PATH_SIZE && is_file(vscode->command) && executable_beside(vscode)) {
            return true;
        }
    }
    return false;
}

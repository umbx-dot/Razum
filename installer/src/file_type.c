#include "setup.h"

#include <windows.h>
#include <shlobj.h>

#define CLASSES L"Software\\Classes\\"
#define PROG_ID L"razum.source"
#define FILE_EXTS L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.rz\\"

static bool delete_tree(const wchar_t *key) {
    LSTATUS status = RegDeleteTreeW(HKEY_CURRENT_USER, key);
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
}

bool register_file_type(const InstallPaths *paths, const VsCode *vscode) {
    wchar_t icon[PATH_SIZE];
    wchar_t command[2 * PATH_SIZE];
    swprintf(icon, PATH_SIZE, L"%ls\\rz.ico", paths->root);
    if (vscode != NULL) {
        swprintf(command, 2 * PATH_SIZE, L"\"%ls\" \"%%1\"", vscode->executable);
    } else {
        swprintf(command, 2 * PATH_SIZE, L"notepad.exe \"%%1\"");
    }
    bool registered = set_user_string(CLASSES L".rz", NULL, PROG_ID) &&
                      set_user_string(CLASSES PROG_ID, NULL, L"Razum Source File") &&
                      set_user_string(CLASSES PROG_ID L"\\DefaultIcon", NULL, icon) &&
                      set_user_string(CLASSES PROG_ID L"\\shell\\open\\command", NULL, command) &&
                      RegSetKeyValueW(HKEY_CURRENT_USER, CLASSES L".rz\\OpenWithProgids", PROG_ID, REG_NONE, NULL, 0) == ERROR_SUCCESS;
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    return registered;
}

bool forget_open_with_choice(void) {
    bool forgotten = delete_tree(FILE_EXTS L"UserChoice") && delete_tree(FILE_EXTS L"UserChoiceLatest");
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    return forgotten;
}

void unregister_file_type(void) {
    wchar_t current[PATH_SIZE];
    DWORD size = sizeof current;
    if (RegGetValueW(HKEY_CURRENT_USER, CLASSES L".rz", NULL, RRF_RT_REG_SZ, NULL, current, &size) == ERROR_SUCCESS &&
        wcscmp(current, PROG_ID) == 0) {
        delete_tree(CLASSES L".rz");
    }
    delete_tree(CLASSES PROG_ID);
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

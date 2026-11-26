#include "setup.h"

#include <windows.h>

#include "resource_ids.h"

static void create_directory(const wchar_t *path) {
    if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        fail(L"Could not create the folder\n%ls", path);
    }
}

static void extract(int id, const wchar_t *directory, const wchar_t *name) {
    wchar_t path[PATH_SIZE];
    swprintf(path, PATH_SIZE, L"%ls\\%ls", directory, name);
    if (!extract_resource(id, path)) {
        fail(L"Could not write\n%ls\n\nIf a razum program or rzc is still running, close it and run setup again.", path);
    }
}

static void copy_uninstaller(const InstallPaths *paths) {
    wchar_t self[PATH_SIZE];
    wchar_t target[PATH_SIZE];
    DWORD length = GetModuleFileNameW(NULL, self, PATH_SIZE);
    swprintf(target, PATH_SIZE, L"%ls\\uninstall.exe", paths->root);
    if (length == 0 || length >= PATH_SIZE || (_wcsicmp(self, target) != 0 && !CopyFileW(self, target, FALSE))) {
        fail(L"Could not write\n%ls", target);
    }
}

static void note(wchar_t *notes, const wchar_t *text) {
    size_t used = wcslen(notes);
    swprintf(notes + used, MESSAGE_SIZE - used, L"\n\n%ls", text);
}

static void offer_vscode_extension(const InstallPaths *paths, wchar_t *notes) {
    VsCode vscode;
    if (!find_vscode(&vscode)) {
        note(notes, L"Visual Studio Code was not found. To add the razum extension later, run:\n    code --install-extension \"%LOCALAPPDATA%\\Programs\\razum\\razum.vsix\"");
        return;
    }
    if (!ask(L"Install the razum extension for Visual Studio Code?\n\nIt adds highlighting, every error while you type, hover, Go to Definition, Run (F5), auto-closing brackets and snippets for .rz files.")) {
        return;
    }
    wchar_t command[3 * PATH_SIZE];
    swprintf(command, 3 * PATH_SIZE, L"cmd.exe /c \"\"%ls\" --install-extension \"%ls\\razum.vsix\" --force\"", vscode.command, paths->root);
    if (run_hidden(command) == 0) {
        note(notes, L"The VS Code extension is installed. Reload open VS Code windows (Developer: Reload Window).");
    } else {
        note(notes, L"The VS Code extension could not be installed. Run:\n    code --install-extension \"%LOCALAPPDATA%\\Programs\\razum\\razum.vsix\"");
    }
}

void install(bool include_vscode) {
    InstallPaths paths;
    if (!install_paths(&paths)) {
        fail(L"Could not find your local application data folder (%%LOCALAPPDATA%%).");
    }
    wchar_t message[MESSAGE_SIZE];
    swprintf(message, MESSAGE_SIZE,
             L"Install razum %ls into\n%ls ?\n\nSetup adds rzc and rz to your PATH and gives .rz files the razum icon. It changes nothing outside your user account.",
             RAZUM_VERSION, paths.root);
    if (!ask(message)) {
        return;
    }
    create_directory(paths.root);
    create_directory(paths.bin);
    create_directory(paths.cmake);
    extract(RESOURCE_RZC, paths.bin, L"rzc.exe");
    extract(RESOURCE_RZ, paths.bin, L"rz.exe");
    extract(RESOURCE_TOML_LICENSE, paths.root, L"tomlc17-LICENSE.txt");
    extract(RESOURCE_NASM, paths.bin, L"nasm.exe");
    extract(RESOURCE_NASM_LICENSE, paths.bin, L"nasm-LICENSE.txt");
    extract(RESOURCE_FILE_ICON, paths.root, L"rz.ico");
    extract(RESOURCE_ICON_LICENSE, paths.root, L"rz-icon-LICENSE.txt");
    extract(RESOURCE_RAZUM_LICENSE, paths.root, L"LICENSE.txt");
    extract(RESOURCE_GUI_NOTICES, paths.root, L"linux-gui-NOTICES.txt");
    extract(RESOURCE_CMAKE_PACKAGE, paths.cmake, L"RazumConfig.cmake");
    extract(RESOURCE_VSIX, paths.root, L"razum.vsix");
    copy_uninstaller(&paths);
    if (!add_to_user_path(paths.bin)) {
        fail(L"Could not add\n%ls\nto your PATH.", paths.bin);
    }
    wchar_t notes[MESSAGE_SIZE] = L"";
    VsCode vscode;
    bool has_vscode = find_vscode(&vscode);
    if (!register_file_type(&paths, has_vscode ? &vscode : NULL)) {
        note(notes, L"The .rz file type could not be registered, so .rz files keep their old icon.");
    } else if (!forget_open_with_choice()) {
        note(notes, L"Windows kept your earlier \"always open .rz files with\" choice, which hides the razum icon. Right-click a .rz file, choose Open with > Choose another app, pick Visual Studio Code and tick \"Always\".");
    }
    if (!register_cmake_package(&paths)) {
        note(notes, L"CMake was not told where razum is; use find_package(Razum) with -DRazum_DIR=%LOCALAPPDATA%\\Programs\\razum\\cmake.");
    }
    if (!register_uninstall_entry(&paths)) {
        note(notes, L"razum could not be added to Settings > Apps; uninstall by running uninstall.exe in the install folder.");
    }
    if (include_vscode) {
        offer_vscode_extension(&paths, notes);
    }
    swprintf(message, MESSAGE_SIZE,
             L"razum %ls is installed.\n\nOpen a new terminal and try:\n    rzc hello.rz --run\n\nIn CMake projects: find_package(Razum) then add_razum_library / add_razum_executable.\n\nUninstall it from Settings > Apps > Installed apps.%ls",
             RAZUM_VERSION, notes);
    inform(message);
}

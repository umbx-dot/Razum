#include "setup.h"

#include <windows.h>

static void delete_installed(const wchar_t *directory, const wchar_t *name) {
    wchar_t path[PATH_SIZE];
    swprintf(path, PATH_SIZE, L"%ls\\%ls", directory, name);
    DeleteFileW(path);
}

static void remove_folder_after_exit(const InstallPaths *paths) {
    wchar_t command[3 * PATH_SIZE];
    swprintf(command, 3 * PATH_SIZE,
             L"cmd.exe /c ping -n 3 127.0.0.1 >nul & del \"%ls\\uninstall.exe\" & rmdir \"%ls\"", paths->root, paths->root);
    STARTUPINFOW startup = {0};
    startup.cb = sizeof startup;
    PROCESS_INFORMATION process = {0};
    if (CreateProcessW(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startup, &process)) {
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }
}

void uninstall(void) {
    InstallPaths paths;
    if (!install_paths(&paths)) {
        fail(L"Could not find your local application data folder (%%LOCALAPPDATA%%).");
    }
    if (!ask(L"Remove razum from this computer?")) {
        return;
    }
    bool path_removed = remove_from_user_path(paths.bin);
    unregister_file_type();
    unregister_uninstall_entry();
    unregister_cmake_package();
    VsCode vscode;
    if (find_vscode(&vscode)) {
        wchar_t command[2 * PATH_SIZE];
        swprintf(command, 2 * PATH_SIZE, L"cmd.exe /c \"\"%ls\" --uninstall-extension idom.razum\"", vscode.command);
        run_hidden(command);
    }
    delete_installed(paths.bin, L"rzc.exe");
    delete_installed(paths.bin, L"rz.exe");
    delete_installed(paths.bin, L"nasm.exe");
    delete_installed(paths.bin, L"nasm-LICENSE.txt");
    RemoveDirectoryW(paths.bin);
    delete_installed(paths.cmake, L"RazumConfig.cmake");
    RemoveDirectoryW(paths.cmake);
    delete_installed(paths.root, L"rz.ico");
    delete_installed(paths.root, L"rz-icon-LICENSE.txt");
    delete_installed(paths.root, L"LICENSE.txt");
    delete_installed(paths.root, L"tomlc17-LICENSE.txt");
    delete_installed(paths.root, L"linux-gui-NOTICES.txt");
    delete_installed(paths.root, L"razum.vsix");
    wchar_t message[MESSAGE_SIZE];
    if (path_removed) {
        swprintf(message, MESSAGE_SIZE, L"razum was removed.");
    } else {
        swprintf(message, MESSAGE_SIZE, L"razum was removed, but\n%ls\nis still in your PATH; remove it in Settings > System > About > Advanced system settings > Environment Variables.", paths.bin);
    }
    inform(message);
    remove_folder_after_exit(&paths);
}

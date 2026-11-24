#include "setup.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command_line, int show) {
    (void)instance;
    (void)previous;
    (void)show;
    set_quiet(wcsstr(command_line, L"--quiet") != NULL);
    if (wcsstr(command_line, L"--uninstall") != NULL) {
        uninstall();
    } else {
        install(wcsstr(command_line, L"--no-vscode") == NULL);
    }
    return 0;
}

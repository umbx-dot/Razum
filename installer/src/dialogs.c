#include "setup.h"

#include <stdarg.h>
#include <stdlib.h>
#include <windows.h>

static const wchar_t TITLE[] = L"razum setup";
static bool quiet = false;

void set_quiet(bool is_quiet) {
    quiet = is_quiet;
}

static void write_console(const wchar_t *text) {
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == NULL || output == INVALID_HANDLE_VALUE) {
        AttachConsole(ATTACH_PARENT_PROCESS);
        output = CreateFileW(L"CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (output == INVALID_HANDLE_VALUE) {
            return;
        }
    }
    char bytes[3 * MESSAGE_SIZE];
    int length = WideCharToMultiByte(CP_UTF8, 0, text, -1, bytes, (int)sizeof bytes - 2, NULL, NULL);
    if (length <= 0) {
        return;
    }
    bytes[length - 1] = '\r';
    bytes[length] = '\n';
    DWORD written = 0;
    WriteFile(output, bytes, (DWORD)length + 1, &written, NULL);
}

bool ask(const wchar_t *question) {
    if (quiet) {
        return true;
    }
    return MessageBoxW(NULL, question, TITLE, MB_YESNO | MB_ICONQUESTION) == IDYES;
}

void inform(const wchar_t *message) {
    if (quiet) {
        write_console(message);
        return;
    }
    MessageBoxW(NULL, message, TITLE, MB_OK | MB_ICONINFORMATION);
}

noreturn void fail(const wchar_t *format, ...) {
    wchar_t message[MESSAGE_SIZE];
    va_list arguments;
    va_start(arguments, format);
    vswprintf(message, MESSAGE_SIZE, format, arguments);
    va_end(arguments);
    if (quiet) {
        write_console(message);
    } else {
        MessageBoxW(NULL, message, TITLE, MB_OK | MB_ICONERROR);
    }
    exit(1);
}

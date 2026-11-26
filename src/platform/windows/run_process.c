#include "platform/process.h"

#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "common/text_buffer.h"

static void append_backslashes(TextBuffer *line, size_t count) {
    for (size_t index = 0; index < count; index++) {
        text_append(line, "\\");
    }
}

static void append_quoted(TextBuffer *line, const char *argument) {
    if (argument[0] != '\0' && strpbrk(argument, " \t\"") == NULL) {
        text_append(line, argument);
        return;
    }
    text_append(line, "\"");
    for (const char *cursor = argument;; cursor++) {
        size_t backslashes = 0;
        while (*cursor == '\\') {
            backslashes++;
            cursor++;
        }
        if (*cursor == '\0') {
            append_backslashes(line, backslashes * 2);
            break;
        }
        if (*cursor == '"') {
            append_backslashes(line, backslashes * 2 + 1);
        } else {
            append_backslashes(line, backslashes);
        }
        text_append_bytes(line, cursor, 1);
    }
    text_append(line, "\"");
}

int run_process_output(const char *const *arguments, const char *directory, const char *output) {
    TextBuffer line = {0};
    for (int index = 0; arguments[index] != NULL; index++) {
        if (index > 0) {
            text_append(&line, " ");
        }
        append_quoted(&line, arguments[index]);
    }
    STARTUPINFOA startup = {0};
    startup.cb = sizeof startup;
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    char application[MAX_PATH];
    DWORD length = GetFullPathNameA(arguments[0], sizeof application, application, NULL);
    if (length == 0 || length >= sizeof application) {
        free(line.data);
        return -1;
    }
    HANDLE capture = INVALID_HANDLE_VALUE;
    if (output != NULL) {
        SECURITY_ATTRIBUTES inheritable = {sizeof inheritable, NULL, TRUE};
        capture = CreateFileA(output, GENERIC_WRITE, FILE_SHARE_READ, &inheritable, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (capture == INVALID_HANDLE_VALUE) {
            free(line.data);
            return -1;
        }
        startup.hStdOutput = capture;
    }
    PROCESS_INFORMATION process = {0};
    BOOL started = CreateProcessA(application, line.data, NULL, NULL, TRUE, 0, NULL, directory, &startup, &process);
    free(line.data);
    if (capture != INVALID_HANDLE_VALUE) {
        CloseHandle(capture);
    }
    if (!started) {
        return -1;
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 1;
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return (int)exit_code;
}

int run_process(const char *const *arguments, const char *directory) {
    return run_process_output(arguments, directory, NULL);
}

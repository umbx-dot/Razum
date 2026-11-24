#include "platform/process.h"

#include <string.h>
#include <windows.h>

static bool is_ascii(const char *text) {
    while (*text != '\0') {
        if ((unsigned char)*text++ > 127) {
            return false;
        }
    }
    return true;
}

int run_assembler(const char *program, const char *assembly, const char *output) {
    if (is_ascii(assembly) && is_ascii(output)) {
        const char *arguments[] = {program, "-f", "bin", assembly, "-o", output, NULL};
        return run_process(arguments, NULL);
    }
    char directory[MAX_PATH];
    char *name = NULL;
    DWORD length = GetFullPathNameA(output, sizeof directory, directory, &name);
    if (length == 0 || length >= sizeof directory || name == NULL) {
        return -1;
    }
    *name = '\0';
    char input_temp[MAX_PATH];
    char output_temp[MAX_PATH];
    if (!GetTempFileNameA(directory, "rza", 0, input_temp)) {
        return -1;
    }
    int status = -1;
    if (GetTempFileNameA(directory, "rzb", 0, output_temp)) {
        if (CopyFileA(assembly, input_temp, FALSE)) {
            const char *arguments[] = {program, "-f", "bin", strrchr(input_temp, '\\') + 1,
                                       "-o", strrchr(output_temp, '\\') + 1, NULL};
            status = run_process(arguments, directory);
            if (status == 0 && !MoveFileExA(output_temp, output, MOVEFILE_REPLACE_EXISTING)) {
                status = -1;
            }
        }
        DeleteFileA(output_temp);
    }
    DeleteFileA(input_temp);
    return status;
}

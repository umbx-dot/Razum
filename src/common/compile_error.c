#include "common/diagnostics.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

const char *diagnostic_source_path = "<input>";

noreturn void compile_error(int line, int column, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    fprintf(stderr, "%s:%d:%d: error: ", diagnostic_source_path, line, column);
    vfprintf(stderr, format, arguments);
    fputc('\n', stderr);
    va_end(arguments);
    diagnostic_count++;
    if (diagnostic_recovery == NULL || diagnostic_count >= DIAGNOSTIC_LIMIT) {
        if (diagnostic_count >= DIAGNOSTIC_LIMIT) {
            fprintf(stderr, "%s: stopping after %d errors\n", diagnostic_source_path, diagnostic_count);
        }
        exit(1);
    }
    longjmp(*diagnostic_recovery, 1);
}

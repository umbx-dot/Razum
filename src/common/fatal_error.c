#include "common/diagnostics.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

noreturn void fatal_error(const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    fputs("rzc: error: ", stderr);
    vfprintf(stderr, format, arguments);
    fputc('\n', stderr);
    va_end(arguments);
    exit(2);
}

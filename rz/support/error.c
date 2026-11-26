#include "project.h"

#include <stdarg.h>
#include <stdlib.h>

noreturn void rz_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fputs("rz: error: ", stderr);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
    exit(2);
}

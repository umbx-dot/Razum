#include "project.h"

#include <stdarg.h>
#include "common/arena.h"

const char *rz_format(const char *format, ...) {
    va_list args;
    va_list copy;
    va_start(args, format);
    va_copy(copy, args);
    int count = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (count < 0) {
        rz_error("cannot format value");
    }
    char *result = arena_alloc((size_t)count + 1);
    vsnprintf(result, (size_t)count + 1, format, args);
    va_end(args);
    return result;
}

#include "common/diagnostics.h"

#include <stddef.h>

jmp_buf *diagnostic_recovery = NULL;
int diagnostic_count = 0;

bool diagnostic_try(void (*work)(void *context), void *context) {
    jmp_buf here;
    jmp_buf *outer = diagnostic_recovery;
    const char *path = diagnostic_source_path;
    diagnostic_recovery = &here;
    if (setjmp(here) == 0) {
        work(context);
        diagnostic_recovery = outer;
        return true;
    }
    diagnostic_source_path = path;
    diagnostic_recovery = outer;
    return false;
}

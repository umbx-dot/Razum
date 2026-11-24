#pragma once

#include <setjmp.h>
#include <stdbool.h>
#include <stdnoreturn.h>

enum { DIAGNOSTIC_LIMIT = 100 };

extern const char *diagnostic_source_path;
extern jmp_buf *diagnostic_recovery;
extern int diagnostic_count;

bool diagnostic_try(void (*work)(void *context), void *context);

noreturn void compile_error(int line, int column, const char *format, ...);
noreturn void fatal_error(const char *format, ...);

#pragma once

#include <stdbool.h>
#include <stddef.h>

int run_process(const char *const *arguments, const char *directory);
bool find_program(const char *name, char *path, size_t size);
long process_id(void);

#ifdef _WIN32
int run_assembler(const char *program, const char *assembly, const char *output);
#endif

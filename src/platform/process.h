#pragma once

#include <stdbool.h>
#include <stddef.h>

int run_process(const char *const *arguments, const char *directory);
int run_process_output(const char *const *arguments, const char *directory, const char *output);
bool find_program(const char *name, char *path, size_t size);
long process_id(void);

int run_assembler(const char *program, const char *assembly, const char *output, const char *format);
#ifndef _WIN32
int run_process_with_input(const char *const *arguments, const char *directory, const char *input);
#endif

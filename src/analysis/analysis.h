#pragma once

#include <stdbool.h>

#include "syntax/syntax.h"

bool always_returns(const Statement *statement);
bool contains_asm(const Statement *statement);
bool runs_on_one_thread(const Program *program, bool is_dll);
void mark_used_functions(Program *program);
void inline_small_call(Expression *call);

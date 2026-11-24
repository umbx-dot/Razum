#pragma once

#include <stdbool.h>

#include "common/list.h"

typedef struct {
    const char *origin;
    const char *text;
} AsmUnit;

typedef struct {
    PointerList units;
    PointerList imports;
} LinkResult;

LinkResult link_units(const char *program_text, const PointerList *available_units, bool imports_exit);
void collect_references(const char *text, const char *prefix, PointerList *names);
bool unit_defines(const AsmUnit *unit, const char *label);

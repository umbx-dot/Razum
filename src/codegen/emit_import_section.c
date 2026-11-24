#include "codegen/codegen.h"

#include <string.h>

#include "common/arena.h"

static const char *dll_of(const char *reference) {
    const char *separator = strchr(reference, '@');
    return separator == NULL ? "kernel32" : arena_copy_text(reference, (size_t)(separator - reference));
}

static const char *function_of(const char *reference) {
    const char *separator = strchr(reference, '@');
    return separator == NULL ? reference : separator + 1;
}

static void collect_dlls(const PointerList *imports, PointerList *dlls) {
    for (int index = 0; index < imports->count; index++) {
        const char *dll = dll_of(LIST_AT(*imports, const char, index));
        bool known = false;
        for (int existing = 0; existing < dlls->count && !known; existing++) {
            known = strcmp(LIST_AT(*dlls, const char, existing), dll) == 0;
        }
        if (!known) {
            list_push(dlls, (void *)dll);
        }
    }
}

void emit_import_section(TextBuffer *output, const PointerList *imports) {
    PointerList dlls = {0};
    collect_dlls(imports, &dlls);
    text_line(output, "");
    text_line(output, "    align 8, db 0");
    for (int dll_index = 0; dll_index < dlls.count; dll_index++) {
        const char *dll = LIST_AT(dlls, const char, dll_index);
        text_line(output, "pe@iat@%s:", dll);
        for (int index = 0; index < imports->count; index++) {
            const char *reference = LIST_AT(*imports, const char, index);
            if (strcmp(dll_of(reference), dll) == 0) {
                text_line(output, "iat@%s: dq name@%s", reference, reference);
            }
        }
        text_line(output, "    dq 0");
    }
    text_line(output, "pe@imports:");
    for (int dll_index = 0; dll_index < dlls.count; dll_index++) {
        const char *dll = LIST_AT(dlls, const char, dll_index);
        text_line(output, "    dd 0, 0, 0, dll@%s, pe@iat@%s", dll, dll);
    }
    text_line(output, "    times 5 dd 0");
    text_line(output, "pe@imports_end:");
    for (int index = 0; index < imports->count; index++) {
        const char *reference = LIST_AT(*imports, const char, index);
        text_line(output, "    align 2, db 0");
        text_line(output, "name@%s: dw 0", reference);
        text_line(output, "    db \"%s\", 0", function_of(reference));
    }
    for (int dll_index = 0; dll_index < dlls.count; dll_index++) {
        const char *dll = LIST_AT(dlls, const char, dll_index);
        text_line(output, "dll@%s: db \"%s.dll\", 0", dll, dll);
    }
}

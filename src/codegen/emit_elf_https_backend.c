#include "codegen/codegen.h"

#if defined(RAZUM_HAS_HTTPS_BACKEND)
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

extern const unsigned char razum_https_backend_start[];
extern const unsigned char razum_https_backend_end[];
#endif

void emit_elf_https_backend(TextBuffer *output) {
    text_line(output, "");
    text_line(output, "elf@https_backend_start:");
#if defined(RAZUM_HAS_HTTPS_BACKEND)
    uintptr_t size = (uintptr_t)razum_https_backend_end - (uintptr_t)razum_https_backend_start;
    for (uintptr_t offset = 0; offset < size; offset += 32) {
        size_t count = size - offset < 32 ? (size_t)(size - offset) : 32u;
        char bytes[sizeof("0x00,") * 32];
        size_t used = 0;
        for (size_t index = 0; index < count; index++) {
            used += (size_t)snprintf(bytes + used, sizeof bytes - used, "%s0x%02X", index == 0 ? "" : ",",
                                     (unsigned int)razum_https_backend_start[offset + index]);
        }
        text_line(output, "    db %s", bytes);
    }
#endif
    text_line(output, "elf@https_backend_end:");
}

#include "codegen/codegen.h"

#include <stdio.h>

const char *relocation_label(ProgramContext *program) {
    static char label[32];
    snprintf(label, sizeof label, "pe@reloc@%d:", ++program->relocation_count);
    return label;
}

void emit_relocations(TextBuffer *output, const ProgramContext *program, bool is_dll) {
    text_line(output, "");
    text_line(output, "    align 4, db 0");
    text_line(output, "pe@relocations:");
    for (int index = 1; is_dll && index <= program->relocation_count; index++) {
        text_line(output, "    dd (pe@reloc@%d - pe@dos) & ~0xFFF, 12", index);
        text_line(output, "    dw 0xA000 | ((pe@reloc@%d - pe@dos) & 0xFFF), 0", index);
    }
    text_line(output, "pe@relocations_end:");
}

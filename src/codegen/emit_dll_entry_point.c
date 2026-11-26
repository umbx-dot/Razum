#include "codegen/codegen.h"

void emit_dll_entry_point(TextBuffer *output, bool flushes_output, bool has_dll_main) {
    text_line(output, "");
    text_line(output, "rz@entry:");
    if (has_dll_main) {
        text_line(output, "    mov edx, edx");
        text_line(output, "    sub rsp, 40");
        if (flushes_output) {
            text_line(output, "    mov dword [rsp + 32], edx");
        }
        text_line(output, "    call $DllMain");
        if (flushes_output) {
            text_line(output, "    cmp dword [rsp + 32], 0");
            text_line(output, "    jne .done");
            text_line(output, "    call rz@flush");
            text_line(output, "    mov eax, 1");
            text_line(output, ".done:");
        }
        text_line(output, "    add rsp, 40");
        text_line(output, "    ret");
        return;
    }
    if (flushes_output) {
        text_line(output, "    test edx, edx");
        text_line(output, "    jnz .done");
        text_line(output, "    sub rsp, 40");
        text_line(output, "    call rz@flush");
        text_line(output, "    add rsp, 40");
        text_line(output, ".done:");
    }
    text_line(output, "    mov eax, 1");
    text_line(output, "    ret");
}

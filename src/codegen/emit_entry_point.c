#include "codegen/codegen.h"

void emit_entry_point(TextBuffer *output, bool flushes_output, bool takes_arguments) {
    text_line(output, "");
    text_line(output, "rz@entry:");
    text_line(output, "    sub rsp, 40");
    if (takes_arguments) {
        text_line(output, "    call rz@arguments");
        text_line(output, "    mov rcx, rax");
    }
    text_line(output, "    call $main");
    if (flushes_output) {
        text_line(output, "    mov dword [rsp + 32], eax");
        text_line(output, "    call rz@flush");
        text_line(output, "    mov eax, dword [rsp + 32]");
    }
    text_line(output, "    mov ecx, eax");
    text_line(output, "    call [iat@ExitProcess]");
}

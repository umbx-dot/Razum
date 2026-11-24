#include "codegen/codegen.h"

void emit_linux_entry_point(TextBuffer *output, bool flushes_output, bool takes_arguments, bool keeps_environment) {
    text_line(output, "");
    text_line(output, "rz@entry:");
    text_line(output, "    mov rcx, rsp");
    if (keeps_environment) {
        text_line(output, "    mov rax, [rcx]");
        text_line(output, "    lea rax, [rcx + rax*8 + 16]");
        text_line(output, "    mov [rz@environment], rax");
    }
    text_line(output, "    and rsp, -16");
    text_line(output, "    sub rsp, 32");
    if (takes_arguments) {
        text_line(output, "    call rz@arguments");
        text_line(output, "    mov rcx, rax");
    }
    text_line(output, "    call $main");
    if (flushes_output) {
        text_line(output, "    mov dword [rsp + 16], eax");
        text_line(output, "    call rz@flush");
        text_line(output, "    mov eax, dword [rsp + 16]");
    }
    text_line(output, "    mov edi, eax");
    text_line(output, "    mov eax, 231                            ; exit_group");
    text_line(output, "    syscall");
}

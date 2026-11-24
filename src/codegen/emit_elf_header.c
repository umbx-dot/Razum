#include "codegen/codegen.h"

static const char ELF_HEADER[] =
    "bits 64\n"
    "default rel\n"
    "org 0\n"
    "\n"
    "pe@base equ 0x400000                       ; load address of the image (named as on Windows)\n"
    "elf@header:\n"
    "    db 0x7F, \"ELF\", 2, 1, 1, 0            ; 64-bit, little endian, version 1, System V\n"
    "    times 8 db 0\n"
    "    dw 2                                    ; executable\n"
    "    dw 0x3E                                 ; machine: x86-64\n"
    "    dd 1                                    ; version\n"
    "    dq pe@base + rz@entry                   ; entry point\n"
    "    dq elf@segments - elf@header            ; program headers\n"
    "    dq 0                                    ; no section headers\n"
    "    dd 0                                    ; flags\n"
    "    dw 64, 56, 2                            ; header size, program header size and count\n"
    "    dw 64, 0, 0                             ; section header size, count, name index\n"
    "elf@segments:\n"
    "    dd 1, 7                                 ; loaded segment: read, write, execute\n"
    "    dq 0, pe@base, pe@base                  ; from file offset 0 to the load address\n"
    "    dq elf@end, elf@end                     ; size in the file and in memory\n"
    "    dq 0x1000                               ; alignment\n"
    "    dd 0x6474E551, 6                        ; stack: read and write, not executable\n"
    "    dq 0, 0, 0, 0, 0, 16\n";

void emit_elf_header(TextBuffer *output) {
    text_append(output, ELF_HEADER);
}

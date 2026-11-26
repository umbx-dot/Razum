bits 64
section .text align=16
global asm_step

asm_step:
%ifidn __OUTPUT_FORMAT__, elf64
    lea eax, [rdi + 6]
    ret
section .note.GNU-stack noalloc noexec nowrite progbits
%else
    lea eax, [rcx + 6]
    ret
%endif

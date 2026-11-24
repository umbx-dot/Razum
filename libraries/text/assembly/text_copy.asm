rz@text_copy:
    xor eax, eax
    test rdx, rdx
    jz .done
    dec rdx
    test r8, r8
    jz .end
.next:
    cmp rax, rdx
    jae .end
    mov r9b, [r8 + rax]
    test r9b, r9b
    jz .end
    mov [rcx + rax], r9b
    inc rax
    jmp .next
.end:
    mov byte [rcx + rax], 0
.done:
    ret

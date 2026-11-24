rz@text_length:
    xor eax, eax
    test rcx, rcx
    jz .done
.next:
    cmp byte [rcx + rax], 0
    je .done
    inc rax
    jmp .next
.done:
    ret

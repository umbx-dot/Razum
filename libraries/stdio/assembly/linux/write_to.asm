rz@write_to:
    push rsi
    push rdi
    mov rdi, rcx
    mov rsi, rdx
    mov rdx, r8
.again:
    test rdx, rdx
    jz .done
    mov eax, 1
    syscall
    cmp rax, -4
    je .again
    test rax, rax
    jle .done
    add rsi, rax
    sub rdx, rax
    jmp .again
.done:
    pop rdi
    pop rsi
    ret

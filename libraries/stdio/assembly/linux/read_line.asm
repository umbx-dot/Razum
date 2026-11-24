rz@read_line:
    push rbx
    push rsi
    push rdi
    push r12
    sub rsp, 40
    mov r12, rcx
    lea rax, [rdx - 1]
    mov [rsp + 32], rax
    xor ebx, ebx
    call rz@lock_output
    call rz@flush
    call rz@unlock_output
.next:
    xor edi, edi
    lea rsi, [rsp + 24]
    mov edx, 1
    xor eax, eax
    syscall
    cmp rax, -4
    je .next
    test rax, rax
    jle .end
    movzx eax, byte [rsp + 24]
    cmp al, 10
    je .done
    cmp al, 13
    je .next
    cmp rbx, [rsp + 32]
    jae .next
    mov [r12 + rbx], al
    inc rbx
    jmp .next
.end:
    test rbx, rbx
    jnz .done
    mov byte [r12], 0
    mov rax, -1
    jmp .return
.done:
    mov byte [r12 + rbx], 0
    mov rax, rbx
.return:
    add rsp, 40
    pop r12
    pop rdi
    pop rsi
    pop rbx
    ret

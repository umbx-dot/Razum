rz@read_line:
    push rbx
    push rsi
    push rdi
    push r12
    sub rsp, 56
    mov rsi, rcx
    lea rdi, [rdx - 1]
    xor ebx, ebx
    call rz@lock_output
    call rz@flush
    call rz@unlock_output
    mov ecx, -10
    call [iat@GetStdHandle]
    mov r12, rax
    mov rcx, r12
    lea rdx, [rsp + 44]
    call [iat@GetConsoleMode]
    test eax, eax
    jz .next
    mov rcx, rsi
    lea rdx, [rdi + 1]
    mov r8, r12
    call rz@read_console_line
    jmp .return
.next:
    mov rcx, r12
    lea rdx, [rsp + 48]
    mov r8d, 1
    lea r9, [rsp + 40]
    mov qword [rsp + 32], 0
    call [iat@ReadFile]
    test eax, eax
    jz .end
    cmp dword [rsp + 40], 0
    je .end
    movzx eax, byte [rsp + 48]
    cmp al, 10
    je .done
    cmp al, 13
    je .next
    cmp rbx, rdi
    jae .next
    mov [rsi + rbx], al
    inc rbx
    jmp .next
.end:
    test rbx, rbx
    jnz .done
    mov byte [rsi], 0
    mov rax, -1
    jmp .return
.done:
    mov byte [rsi + rbx], 0
    mov rax, rbx
.return:
    add rsp, 56
    pop r12
    pop rdi
    pop rsi
    pop rbx
    ret

rz@write_newline:
    sub rsp, 120
    lea rcx, [rz@newline]
    mov edx, 1
    call rz@write
    mov rax, [rz@output_console]
    test rax, rax
    jnz .known
    mov [rsp + 96], rdi
    mov [rsp + 104], rsi
    mov edi, 1
    mov esi, 0x5401
    lea rdx, [rsp + 32]
    mov eax, 16
    syscall
    mov rdi, [rsp + 96]
    mov rsi, [rsp + 104]
    test rax, rax
    mov eax, 2
    jnz .store
    mov eax, 1
.store:
    mov [rz@output_console], rax
.known:
    cmp rax, 1
    jne .done
    call rz@flush
.done:
    add rsp, 120
    ret

rz@newline: db 10

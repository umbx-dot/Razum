rz@write_newline:
    sub rsp, 40
    lea rcx, [rz@newline]
    mov edx, 2
    call rz@write
    mov rax, [rz@output_console]
    test rax, rax
    jnz .known
    mov ecx, -11
    call [iat@GetStdHandle]
    mov rcx, rax
    lea rdx, [rsp + 32]
    call [iat@GetConsoleMode]
    test eax, eax
    mov eax, 2
    jz .store
    mov eax, 1
.store:
    mov [rz@output_console], rax
.known:
    cmp rax, 1
    jne .done
    call rz@flush
.done:
    add rsp, 40
    ret

rz@newline: db 13, 10

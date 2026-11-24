rz@write_int:
    sub rsp, 72
    mov [rsp + 64], rdi
    mov rax, rcx
    lea rdi, [rsp + 64]
    call rz@format_int
    lea rdx, [rsp + 64]
    sub rdx, rdi
    mov rcx, rdi
    call rz@write
    mov rdi, [rsp + 64]
    add rsp, 72
    ret

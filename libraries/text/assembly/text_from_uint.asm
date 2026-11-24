rz@text_from_uint:
    push rdi
    sub rsp, 48
    mov [rsp + 32], rcx
    mov [rsp + 40], rdx
    mov byte [rsp + 31], 0
    lea rdi, [rsp + 31]
    mov rax, r8
    call rz@format_uint
    mov r8, rdi
    mov rcx, [rsp + 32]
    mov rdx, [rsp + 40]
    call rz@text_copy
    add rsp, 48
    pop rdi
    ret

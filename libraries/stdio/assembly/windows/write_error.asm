rz@write_error:
    push rbx
    sub rsp, 48
    mov rbx, rcx
    call rz@flush
    mov rcx, rbx
    call rz@text_length
    mov [rsp + 40], rax
    mov ecx, -12
    call [iat@GetStdHandle]
    mov rcx, rax
    mov rdx, rbx
    mov r8, [rsp + 40]
    call rz@write_to
    add rsp, 48
    pop rbx
    ret

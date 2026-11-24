rz@write_handle:
    sub rsp, 56
    mov [rsp + 40], rcx
    mov [rsp + 48], rdx
    mov ecx, -11
    call [iat@GetStdHandle]
    mov rcx, rax
    mov rdx, [rsp + 40]
    mov r8, [rsp + 48]
    call rz@write_to
    add rsp, 56
    ret

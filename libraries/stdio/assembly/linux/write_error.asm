rz@write_error:
    push rbx
    sub rsp, 32
    mov rbx, rcx
    call rz@flush
    mov rcx, rbx
    call rz@text_length
    mov r8, rax
    mov rdx, rbx
    mov ecx, 2
    call rz@write_to
    add rsp, 32
    pop rbx
    ret

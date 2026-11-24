rz@write_double:
    mov edx, 15
    jmp rz@write_real
rz@write_float:
    cvtss2sd xmm0, xmm0
    mov edx, 7
rz@write_real:
    sub rsp, 104
    mov [rsp + 96], rdi
    fninit
    lea rdi, [rsp + 32]
    mov ecx, edx
    call rz@format_real
    lea rcx, [rsp + 32]
    mov rdx, rdi
    sub rdx, rcx
    call rz@write
    mov rdi, [rsp + 96]
    add rsp, 104
    ret

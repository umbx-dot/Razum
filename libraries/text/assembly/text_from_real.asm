rz@text_from_real:
    push rdi
    sub rsp, 112
    mov [rsp + 96], rcx
    mov [rsp + 104], rdx
    fninit
    lea rdi, [rsp + 32]
    mov ecx, r8d
    call rz@format_real
    mov byte [rdi], 0
    lea r8, [rsp + 32]
    mov rcx, [rsp + 96]
    mov rdx, [rsp + 104]
    call rz@text_copy
    add rsp, 112
    pop rdi
    ret

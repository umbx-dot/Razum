rz@format_int:
    mov r8, rax
    mov ecx, 10
    test rax, rax
    jns .digit
    neg rax
.digit:
    xor edx, edx
    div rcx
    add dl, '0'
    dec rdi
    mov [rdi], dl
    test rax, rax
    jnz .digit
    test r8, r8
    jns .done
    dec rdi
    mov byte [rdi], '-'
.done:
    ret

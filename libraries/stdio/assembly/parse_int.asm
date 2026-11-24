rz@parse_int:
    xor eax, eax
    xor r8d, r8d
.space:
    movzx edx, byte [rcx]
    cmp dl, ' '
    je .skip
    cmp dl, 9
    jne .sign
.skip:
    inc rcx
    jmp .space
.sign:
    cmp dl, '-'
    jne .plus
    mov r8d, 1
    inc rcx
    jmp .digit
.plus:
    cmp dl, '+'
    jne .digit
    inc rcx
.digit:
    movzx edx, byte [rcx]
    sub edx, '0'
    cmp edx, 9
    ja .done
    imul rax, rax, 10
    add rax, rdx
    inc rcx
    jmp .digit
.done:
    test r8d, r8d
    jz .return
    neg rax
.return:
    ret

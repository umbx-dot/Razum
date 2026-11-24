rz@skip_spaces:
    dec rsi
.next:
    inc rsi
    cmp byte [rsi], ' '
    je .next
    ret

rz@parse_number:
    call rz@skip_spaces
    xor r11d, r11d
    cmp byte [rsi], '-'
    jne .unsigned
    inc r11d
    inc rsi
.unsigned:
    xor eax, eax
    xor r9d, r9d
    xor r10d, r10d
    mov r8, 1844674407370955160
.integer_digit:
    movzx edx, byte [rsi]
    sub edx, '0'
    cmp edx, 9
    ja .integer_end
    or r10d, 1
    inc rsi
    cmp rax, r8
    ja .integer_dropped
    imul rax, rax, 10
    add rax, rdx
    jmp .integer_digit
.integer_dropped:
    inc r9
    jmp .integer_digit
.integer_end:
    cmp byte [rsi], '.'
    jne .exponent
    or r10d, 2
    inc rsi
.fraction_digit:
    movzx edx, byte [rsi]
    sub edx, '0'
    cmp edx, 9
    ja .exponent
    or r10d, 1
    inc rsi
    cmp rax, r8
    ja .fraction_digit
    imul rax, rax, 10
    add rax, rdx
    dec r9
    jmp .fraction_digit
.exponent:
    test r10d, 1
    jz .fail
    movzx edx, byte [rsi]
    or dl, 0x20
    cmp dl, 'e'
    jne .suffix
    or r10d, 2
    inc rsi
    mov dl, [rsi]
    cmp dl, '+'
    je .exponent_sign_read
    cmp dl, '-'
    jne .exponent_first
    or r10d, 4
.exponent_sign_read:
    inc rsi
.exponent_first:
    movzx edx, byte [rsi]
    sub edx, '0'
    cmp edx, 9
    ja .fail
    xor ecx, ecx
.exponent_digit:
    inc rsi
    cmp ecx, 100000
    jae .exponent_next
    imul ecx, ecx, 10
    add ecx, edx
.exponent_next:
    movzx edx, byte [rsi]
    sub edx, '0'
    cmp edx, 9
    jbe .exponent_digit
    test r10d, 4
    jz .exponent_add
    neg rcx
.exponent_add:
    add r9, rcx
.suffix:
    xor ecx, ecx
    test r10d, 2
    jz .suffix_f
    mov ecx, 2
.suffix_f:
    movzx edx, byte [rsi]
    or dl, 0x20
    cmp dl, 'f'
    jne .typed
    mov ecx, 1
    inc rsi
.typed:
    test ecx, ecx
    jnz .real
    test r9, r9
    jnz .fail
    test r11d, r11d
    jnz .negative_integer
    test rax, rax
    js .fail
    jmp .integer_done
.negative_integer:
    mov rdx, 0x8000000000000000
    cmp rax, rdx
    ja .fail
    neg rax
.integer_done:
    cvtsi2sd xmm0, rax
    clc
    ret
.real:
    xorpd xmm0, xmm0
    test rax, rax
    jz .real_sign
    mov r10d, ecx
    jns .mantissa_fits
    xor edx, edx
    mov ecx, 10
    div rcx
    inc r9
.mantissa_fits:
    push rax
    fild qword [rsp]
    mov rcx, r9
    mov rdx, 6000
    cmp rcx, rdx
    cmovg rcx, rdx
    neg rdx
    cmp rcx, rdx
    cmovl rcx, rdx
    call rz@pow10
    fmulp st1, st0
    fstp qword [rsp]
    movsd xmm0, [rsp]
    pop rax
    mov ecx, r10d
.real_sign:
    test r11d, r11d
    jz .real_rounded
    movq rax, xmm0
    btc rax, 63
    movq xmm0, rax
.real_rounded:
    cmp ecx, 1
    jne .real_done
    cvtsd2ss xmm0, xmm0
    cvtss2sd xmm0, xmm0
.real_done:
    clc
    ret
.fail:
    stc
    ret

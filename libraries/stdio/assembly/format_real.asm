rz@format_real:
    push rbx
    push rsi
    push rbp
    sub rsp, 48
    mov ebx, ecx
    movq rax, xmm0
    mov rdx, rax
    btr rdx, 63
    mov rcx, 0x7FF0000000000000
    cmp rdx, rcx
    ja .nan
    test rax, rax
    jns .unsigned
    mov byte [rdi], '-'
    inc rdi
.unsigned:
    cmp rdx, rcx
    je .infinity
    test rdx, rdx
    jz .zero
    mov [rsp], rdx
    fld qword [rsp]
    fldlg2
    fld st1
    fyl2x
    fistp qword [rsp]
    mov rsi, [rsp]
    mov rcx, rsi
    call rz@pow10
    fcomip st0, st1
    jbe .exponent_found
    dec rsi
.exponent_found:
    lea rcx, [rbx - 1]
    sub rcx, rsi
    call rz@pow10
    fmulp st1, st0
    fistp qword [rsp]
    mov rax, [rsp]
    mov ecx, ebx
    mov r8d, 1
.power:
    imul r8, r8, 10
    dec ecx
    jnz .power
    cmp rax, r8
    jb .below_high
    xor edx, edx
    mov ecx, 10
    div rcx
    inc rsi
.below_high:
    imul rcx, rax, 10
    cmp rcx, r8
    jae .digits_ready
    mov rax, rcx
    dec rsi
.digits_ready:
    lea r9, [rsp + 8 + rbx]
    mov r10d, 10
    mov r11d, ebx
.digit:
    xor edx, edx
    div r10
    add dl, '0'
    dec r9
    mov [r9], dl
    dec r11d
    jnz .digit
    mov r11d, ebx
.trim:
    cmp r11d, 1
    je .trimmed
    cmp byte [rsp + 8 + r11 - 1], '0'
    jne .trimmed
    dec r11d
    jmp .trim
.trimmed:
    cmp rsi, -4
    jl .scientific
    cmp rsi, rbx
    jge .scientific
    test rsi, rsi
    js .fixed_small
    xor ecx, ecx
.integer_part:
    mov al, '0'
    cmp rcx, r11
    jae .integer_char
    mov al, [rsp + 8 + rcx]
.integer_char:
    mov [rdi], al
    inc rdi
    inc rcx
    cmp rcx, rsi
    jbe .integer_part
    mov byte [rdi], '.'
    inc rdi
    jmp .fraction

.fixed_small:
    mov word [rdi], '0.'
    add rdi, 2
    mov rcx, rsi
    not rcx
.leading_zero:
    test rcx, rcx
    jz .all_digits
    mov byte [rdi], '0'
    inc rdi
    dec rcx
    jmp .leading_zero
.all_digits:
    xor ecx, ecx
    jmp .fraction

.scientific:
    mov al, [rsp + 8]
    mov [rdi], al
    mov byte [rdi + 1], '.'
    add rdi, 2
    mov ecx, 1
    call .copy_fraction
    mov byte [rdi], 'e'
    inc rdi
    mov rbp, rdi
    mov rax, rsi
    lea rdi, [rsp + 48]
    call rz@format_int
    lea rcx, [rsp + 48]
.exponent_char:
    mov al, [rdi]
    mov [rbp], al
    inc rdi
    inc rbp
    cmp rdi, rcx
    jb .exponent_char
    mov rdi, rbp
    jmp .done

.fraction:
    call .copy_fraction
    jmp .done

.copy_fraction:
    cmp rcx, r11
    jb .fraction_char
    mov byte [rdi], '0'
    inc rdi
    ret
.fraction_char:
    mov al, [rsp + 16 + rcx]
    mov [rdi], al
    inc rdi
    inc rcx
    cmp rcx, r11
    jb .fraction_char
    ret

.nan:
    mov dword [rdi], 'nan'
    add rdi, 3
    jmp .done
.infinity:
    mov dword [rdi], 'inf'
    add rdi, 3
    jmp .done
.zero:
    mov dword [rdi], '0.0'
    add rdi, 3
.done:
    add rsp, 48
    pop rbp
    pop rsi
    pop rbx
    ret

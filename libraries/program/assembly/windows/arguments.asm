rz@arguments:
    push rbx
    push rsi
    push rdi
    sub rsp, 80
    call [iat@GetCommandLineW]
    mov rbx, rax
    mov ecx, 65001
    xor edx, edx
    mov r8, rbx
    mov r9d, -1
    mov qword [rsp + 32], 0
    mov qword [rsp + 40], 0
    mov qword [rsp + 48], 0
    mov qword [rsp + 56], 0
    call [iat@WideCharToMultiByte]
    mov [rsp + 64], rax
    xor ecx, ecx
    mov rdx, rax
    mov r8d, 0x3000
    mov r9d, 4
    call [iat@VirtualAlloc]
    mov rsi, rax
    mov ecx, 65001
    xor edx, edx
    mov r8, rbx
    mov r9d, -1
    mov [rsp + 32], rsi
    mov rax, [rsp + 64]
    mov [rsp + 40], eax
    mov qword [rsp + 48], 0
    mov qword [rsp + 56], 0
    call [iat@WideCharToMultiByte]
    xor ebx, ebx
.measure:
    cmp byte [rsi + rbx], 0
    je .allocate
    inc rbx
    jmp .measure
.allocate:
    mov rdi, rbx
    shr rdi, 1
    add rdi, 2
    lea rdx, [rbx + rdi*8 + 1]
    xor ecx, ecx
    mov r8d, 0x3000
    mov r9d, 4
    call [iat@VirtualAlloc]
    mov r10, rax
    lea rdi, [rax + rdi*8]
    xor r11d, r11d
.between:
    mov al, [rsi]
    cmp al, ' '
    je .skip
    cmp al, 9
    je .skip
    test al, al
    jz .finish
    mov [r10 + r11*8], rdi
    inc r11
    xor edx, edx
.character:
    mov al, [rsi]
    test al, al
    jz .end_argument
    cmp al, '"'
    jne .plain
    xor edx, 1
    inc rsi
    jmp .character
.plain:
    test edx, edx
    jnz .store
    cmp al, ' '
    je .end_argument
    cmp al, 9
    je .end_argument
.store:
    mov [rdi], al
    inc rdi
    inc rsi
    jmp .character
.end_argument:
    mov byte [rdi], 0
    inc rdi
    jmp .between
.skip:
    inc rsi
    jmp .between
.finish:
    mov rax, r10
    mov rdx, r11
    add rsp, 80
    pop rdi
    pop rsi
    pop rbx
    ret

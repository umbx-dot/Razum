rz@read_console_line:
    push rbx
    push rsi
    push rdi
    push r12
    push r13
    push r14
    sub rsp, 88
    mov rsi, rcx
    lea rdi, [rdx - 1]
    mov r12, r8
    xor ebx, ebx
    xor r13d, r13d
    mov rax, [rz@wide_input]
    test rax, rax
    jnz .read
    xor ecx, ecx
    mov edx, 16384
    mov r8d, 0x3000
    mov r9d, 4
    call [iat@VirtualAlloc]
    test rax, rax
    jz .end_of_input
    mov [rz@wide_input], rax
.read:
    mov rcx, r12
    mov rdx, [rz@wide_input]
    mov r8d, 8192
    lea r9, [rsp + 64]
    mov qword [rsp + 64], 0
    mov qword [rsp + 32], 0
    call [iat@ReadConsoleW]
    test eax, eax
    jz .end_of_input
    mov eax, dword [rsp + 64]
    test eax, eax
    jz .end_of_input
    mov r14, [rz@wide_input]
    lea rax, [r14 + rax*2]
    mov [rsp + 64], rax
    test r13d, r13d
    jnz .unit
    cmp word [r14], 26
    je .end_of_input
    mov r13d, 1
.unit:
    cmp r14, [rsp + 64]
    jae .read
    movzx eax, word [r14]
    cmp eax, 10
    je .finish
    cmp eax, 13
    je .skip
    mov dword [rsp + 72], 1
    cmp eax, 0xD800
    jb .convert
    cmp eax, 0xDBFF
    ja .convert
    lea rdx, [r14 + 4]
    cmp rdx, [rsp + 64]
    ja .convert
    mov dword [rsp + 72], 2
.convert:
    mov ecx, 65001
    xor edx, edx
    mov r8, r14
    mov r9d, dword [rsp + 72]
    lea rax, [rsp + 80]
    mov [rsp + 32], rax
    mov dword [rsp + 40], 8
    mov qword [rsp + 48], 0
    mov qword [rsp + 56], 0
    call [iat@WideCharToMultiByte]
    test eax, eax
    jz .advance
    lea rdx, [rbx + rax]
    cmp rdx, rdi
    ja .advance
    xor ecx, ecx
.copy:
    mov dl, [rsp + 80 + rcx]
    mov [rsi + rbx], dl
    inc rbx
    inc ecx
    cmp ecx, eax
    jb .copy
.advance:
    mov eax, dword [rsp + 72]
    lea r14, [r14 + rax*2]
    jmp .unit
.skip:
    add r14, 2
    jmp .unit
.end_of_input:
    test r13d, r13d
    jnz .finish
    test rbx, rbx
    jnz .finish
    mov byte [rsi], 0
    mov rax, -1
    jmp .return
.finish:
    mov byte [rsi + rbx], 0
    mov rax, rbx
.return:
    add rsp, 88
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbx
    ret

    align 8, db 0
rz@wide_input: dq 0

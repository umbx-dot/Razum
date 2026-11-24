rz@write_to:
    push rbx
    push rsi
    push rdi
    push r12
    sub rsp, 56
    mov rbx, rcx
    mov rsi, rdx
    mov rdi, r8
    mov rcx, rbx
    lea rdx, [rsp + 48]
    call [iat@GetConsoleMode]
    test eax, eax
    jz .file
    mov rax, [rz@wide_output]
    test rax, rax
    jnz .console
    xor ecx, ecx
    mov edx, 65536
    mov r8d, 0x3000
    mov r9d, 4
    call [iat@VirtualAlloc]
    test rax, rax
    jz .file
    mov [rz@wide_output], rax
.console:
    test rdi, rdi
    jz .done
    mov r12, rdi
    cmp r12, 16384
    jbe .convert
    mov r12, 16384
    lea r10, [r12 - 3]
.boundary:
    movzx eax, byte [rsi + r12]
    and eax, 0xC0
    cmp eax, 0x80
    jne .convert
    dec r12
    cmp r12, r10
    ja .boundary
    mov r12, 16384
.convert:
    mov ecx, 65001
    xor edx, edx
    mov r8, rsi
    mov r9d, r12d
    mov rax, [rz@wide_output]
    mov [rsp + 32], rax
    mov dword [rsp + 40], 32768
    call [iat@MultiByteToWideChar]
    mov rcx, rbx
    mov rdx, [rz@wide_output]
    mov r8d, eax
    lea r9, [rsp + 48]
    mov qword [rsp + 32], 0
    call [iat@WriteConsoleW]
    add rsi, r12
    sub rdi, r12
    jmp .console
.file:
    mov rcx, rbx
    mov rdx, rsi
    mov r8, rdi
    lea r9, [rsp + 48]
    mov qword [rsp + 32], 0
    call [iat@WriteFile]
.done:
    add rsp, 56
    pop r12
    pop rdi
    pop rsi
    pop rbx
    ret

    align 8, db 0
rz@wide_output: dq 0

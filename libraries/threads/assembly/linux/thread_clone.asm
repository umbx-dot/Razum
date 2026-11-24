rz@thread_clone:
    push rsi
    push rdi
    sub rcx, 16
    mov [rcx], r8
    mov [rcx + 8], r9
    mov rsi, rcx
    mov edi, 0x350F00
    mov r10, rdx
    xor r8d, r8d
    mov eax, 56
    syscall
    test rax, rax
    jz .child
    pop rdi
    pop rsi
    ret
.child:
    mov rax, [rsp]
    mov rcx, [rsp + 8]
    and rsp, -16
    sub rsp, 32
    call rax
    xor edi, edi
    mov eax, 60
    syscall

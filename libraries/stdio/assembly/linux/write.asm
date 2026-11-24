rz@write:
    mov rax, [rz@output_used]
    lea r8, [rax + rdx]
    cmp r8, rz@OUTPUT_SIZE
    ja .flush_first
    mov r8, [rz@output_buffer]
    test r8, r8
    jz .allocate
    add r8, rax
    add rax, rdx
    mov [rz@output_used], rax
    test rdx, rdx
    jz .done
.copy:
    mov al, [rcx]
    mov [r8], al
    inc rcx
    inc r8
    dec rdx
    jnz .copy
.done:
    ret
.flush_first:
    sub rsp, 56
    mov [rsp + 32], rcx
    mov [rsp + 40], rdx
    call rz@flush
    mov rcx, [rsp + 32]
    mov rdx, [rsp + 40]
    add rsp, 56
    cmp rdx, rz@OUTPUT_SIZE
    ja rz@write_handle
    jmp rz@write
.allocate:
    push rcx
    push rdx
    push rsi
    push rdi
    xor edi, edi
    mov esi, rz@OUTPUT_SIZE
    mov edx, 3
    mov r10d, 0x22
    mov r8, -1
    xor r9d, r9d
    mov eax, 9
    syscall
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    cmp rax, -4096
    ja rz@write_handle
    mov [rz@output_buffer], rax
    jmp rz@write

rz@release:
    test rcx, rcx
    jz .done
    lea rdx, [rcx - 16]
    mov rax, [rdx + 8]
    cmp rax, -1
    je .large
.lock:
    mov r8d, 1
    xchg r8d, [rz@heap_lock]
    test r8d, r8d
    jz .locked
    pause
    jmp .lock
.locked:
    lea r8, [rz@free_lists]
    mov r9, [r8 + rax*8]
    mov [rdx], r9
    mov [r8 + rax*8], rdx
    mov dword [rz@heap_lock], 0
    ret
.large:
    push rsi
    push rdi
    mov rdi, rdx
    mov rsi, [rdx]
    mov eax, 11
    syscall
    pop rdi
    pop rsi
.done:
    ret

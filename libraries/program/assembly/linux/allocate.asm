rz@allocate:
    push rbx
    push rsi
    push rdi
    push r12
    lea rax, [rcx + 16]
    cmp rax, 32768
    ja .large
    mov r12d, 16
    xor ebx, ebx
.class:
    cmp r12, rax
    jae .lock
    shl r12, 1
    inc ebx
    jmp .class
.lock:
    mov eax, 1
    xchg eax, [rz@heap_lock]
    test eax, eax
    jz .locked
    pause
    jmp .lock
.locked:
    lea rdx, [rz@free_lists]
    mov rax, [rdx + rbx*8]
    test rax, rax
    jz .carve
    mov rcx, [rax]
    mov [rdx + rbx*8], rcx
    jmp .got
.carve:
    mov rax, [rz@heap_next]
    mov rcx, [rz@heap_end]
    sub rcx, rax
    cmp rcx, r12
    jae .take
    xor edi, edi
    mov esi, 1048576
    mov edx, 3
    mov r10d, 0x22
    mov r8, -1
    xor r9d, r9d
    mov eax, 9
    syscall
    cmp rax, -4096
    ja .failed_locked
    lea rcx, [rax + 1048576]
    mov [rz@heap_end], rcx
.take:
    lea rcx, [rax + r12]
    mov [rz@heap_next], rcx
.got:
    mov dword [rz@heap_lock], 0
    mov [rax], r12
    mov [rax + 8], rbx
    mov rdx, rax
    lea rdi, [rax + 16]
    lea rcx, [r12 - 16]
    xor eax, eax
    rep stosb
    lea rax, [rdx + 16]
    jmp .return
.large:
    lea rbx, [rax + 4095]
    and rbx, -4096
    xor edi, edi
    mov rsi, rbx
    mov edx, 3
    mov r10d, 0x22
    mov r8, -1
    xor r9d, r9d
    mov eax, 9
    syscall
    cmp rax, -4096
    ja .failed
    mov [rax], rbx
    mov qword [rax + 8], -1
    add rax, 16
    jmp .return
.failed_locked:
    mov dword [rz@heap_lock], 0
.failed:
    xor eax, eax
.return:
    pop r12
    pop rdi
    pop rsi
    pop rbx
    ret

    align 8, db 0
rz@free_lists: times 12 dq 0
rz@heap_next: dq 0
rz@heap_end: dq 0
rz@heap_lock: dd 0

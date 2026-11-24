rz@allocate:
    push rbx
    sub rsp, 32
    mov rbx, rcx
    call [iat@GetProcessHeap]
    mov rcx, rax
    mov edx, 8
    mov r8, rbx
    call [iat@HeapAlloc]
    add rsp, 32
    pop rbx
    ret

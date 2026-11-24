rz@release:
    test rcx, rcx
    jz .done
    push rbx
    sub rsp, 32
    mov rbx, rcx
    call [iat@GetProcessHeap]
    mov rcx, rax
    xor edx, edx
    mov r8, rbx
    call [iat@HeapFree]
    add rsp, 32
    pop rbx
.done:
    ret

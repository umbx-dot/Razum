rz@text_append:
    lea r9, [rdx - 1]
    xor eax, eax
.find:
    cmp rax, r9
    jae .found
    cmp byte [rcx + rax], 0
    je .found
    inc rax
    jmp .find
.found:
    push rax
    add rcx, rax
    sub rdx, rax
    call rz@text_copy
    pop rcx
    add rax, rcx
    ret

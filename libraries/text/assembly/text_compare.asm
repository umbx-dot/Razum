rz@text_compare:
    lea r8, [rz@text_empty]
    test rcx, rcx
    cmovz rcx, r8
    test rdx, rdx
    cmovz rdx, r8
.next:
    movzx eax, byte [rcx]
    movzx r9d, byte [rdx]
    cmp eax, r9d
    jne .differ
    test eax, eax
    jz .equal
    inc rcx
    inc rdx
    jmp .next
.differ:
    sbb eax, eax
    or eax, 1
    ret
.equal:
    xor eax, eax
    ret

rz@text_empty: db 0

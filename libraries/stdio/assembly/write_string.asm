rz@write_string:
    xor edx, edx
    test rcx, rcx
    jz .write
.measure:
    cmp byte [rcx + rdx], 0
    je .write
    inc rdx
    jmp .measure
.write:
    jmp rz@write

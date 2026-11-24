rz@pow10:
    mov rax, rcx
    neg rax
    cmovs rax, rcx
    fld1
    fild dword [rz@pow10_base]
.bit:
    test rax, rax
    jz .done
    test al, 1
    jz .square
    fmul st1, st0
.square:
    fmul st0, st0
    shr rax, 1
    jmp .bit
.done:
    fstp st0
    test rcx, rcx
    jns .return
    fld1
    fdivrp st1, st0
.return:
    ret

rz@pow10_base: dd 10

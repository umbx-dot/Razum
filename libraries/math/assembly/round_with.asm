rz@round_with:
    sub rsp, 24
    fnstcw [rsp]
    mov ax, [rsp]
    and ax, 0xF3FF
    shl ecx, 10
    or ax, cx
    mov [rsp + 2], ax
    fldcw [rsp + 2]
    movsd [rsp + 8], xmm0
    fld qword [rsp + 8]
    frndint
    fstp qword [rsp + 8]
    fldcw [rsp]
    movsd xmm0, [rsp + 8]
    add rsp, 24
    ret

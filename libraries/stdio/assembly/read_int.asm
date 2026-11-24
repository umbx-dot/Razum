rz@read_int:
    sub rsp, 296
    lea rcx, [rsp + 32]
    mov edx, 256
    call rz@read_line
    lea rcx, [rsp + 32]
    call rz@parse_int
    add rsp, 296
    ret

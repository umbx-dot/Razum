rz@read_double:
    sub rsp, 296
    lea rcx, [rsp + 32]
    mov edx, 256
    call rz@read_line
    fninit
    lea rcx, [rsp + 32]
    call rz@parse_double
    add rsp, 296
    ret

rz@parse_double:
    push rsi
    mov rsi, rcx
    call rz@parse_number
    jnc .done
    xorpd xmm0, xmm0
.done:
    pop rsi
    ret

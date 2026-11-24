rz@flush:
    mov rdx, [rz@output_used]
    test rdx, rdx
    jz .done
    mov qword [rz@output_used], 0
    mov rcx, [rz@output_buffer]
    jmp rz@write_handle
.done:
    ret

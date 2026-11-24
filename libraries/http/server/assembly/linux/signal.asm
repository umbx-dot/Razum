rz@http_interrupt:
    sub rsp, 40
    mov ecx, 2
    call [$http_interrupt_handler]
    add rsp, 40
    test eax, eax
    jnz .done
    mov edi, 130
    mov eax, 231
    syscall
.done:
    ret

rz@signal_return:
    mov eax, 15
    syscall

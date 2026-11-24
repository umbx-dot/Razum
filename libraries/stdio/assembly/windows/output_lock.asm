rz@lock_output:
    mov eax, 1
    xchg eax, [rz@output_lock]
    test eax, eax
    jnz .contended
    ret
.contended:
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
    sub rsp, 40
.wait:
    mov ecx, 64
.spin:
    pause
    cmp dword [rz@output_lock], 0
    je .try
    dec ecx
    jnz .spin
    call [iat@SwitchToThread]
    jmp .wait
.try:
    mov eax, 1
    xchg eax, [rz@output_lock]
    test eax, eax
    jnz .wait
    add rsp, 40
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    ret

rz@unlock_output:
    mov dword [rz@output_lock], 0
    ret

    align 4, db 0
rz@output_lock: dd 0

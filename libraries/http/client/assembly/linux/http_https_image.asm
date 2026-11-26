rz@http_https_image:
    lea rax, [rel elf@https_backend_start]
    mov qword [rcx], elf@https_backend_end - elf@https_backend_start
    ret

[BITS 64]

section .text
    global linux_x64_exitThread:function
    global linux_x64_exit:function

; [[noreturn]] void linux_x64_exitThread(int error_code);
linux_x64_exitThread:
    mov eax, 60
    syscall

; [[noreturn]] void linux_x64_exit(int error_code);
linux_x64_exit:
    mov eax, 231
    syscall
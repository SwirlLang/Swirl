[BITS 32]

section .text
    global linux_x86_exitThread:function
    global linux_x86_exit:function

; [[noreturn]] void linux_x86_exitThread(int error_code);
linux_x86_exitThread:
    mov eax, 1
    int 0x80

; [[noreturn]] void linux_x86_exit(int error_code);
linux_x64_exit:
    mov eax, 252
    int 0x80
[BITS 32]

section .text
    extern _ExitThread@4:
    extern _ExitProcess@4:
    global windows_x86_exitThread:
    global windows_x86_exit:

; [[noreturn]] void windows_x86_exitThread(uint32_t dwExitCode);
windows_x86_exitThread:
    jmp near _ExitThread@4
    int3

; [[noreturn]] void windows_x86_exit(uint32_t uExitCode);
windows_x86_exit:
    jmp near _ExitProcess@4
    int3
[BITS 64]

section .text
    extern ExitThread:
    extern ExitProcess:
    global windows_x64_exitThread:
    global windows_x64_exit:

; [[noreturn]] void windows_x64_exitThread(uint32_t dwExitCode);
windows_x64_exitThread:
    jmp near ExitThread
    int3

; [[noreturn]] void windows_x64_exit(uint32_t uExitCode);
windows_x64_exit:
    jmp near ExitProcess
    int3
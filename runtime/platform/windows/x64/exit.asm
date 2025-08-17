[BITS 64]

section .text
    extern ExitThread:
    extern ExitProcess:
    global windows_x64_exitThread:
    global windows_x64_exit:

; [[noreturn]] void windows_x64_exitThread(uint32_t dwExitCode);
windows_x64_exitThread:
    sub rsp, 40
    call ExitThread

; [[noreturn]] void windows_x64_exit(uint32_t uExitCode);
windows_x64_exit:
    sub rsp, 40
    call ExitProcess
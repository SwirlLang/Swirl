[BITS 64]

section .text
    extern main:
    extern GetCommandLineW:
    extern CommandLineToArgvW:
    extern WideCharToMultiByte:
    extern VirtualAlloc:
    extern LocalFree:
    extern ExitProcess:
    global _start:
    global __main:

_start:
    and rsp, -16 ; maintain stack alignment but let functions use _start's shadow space since it's not used
    mov rbp, rsp
    call GetCommandLineW

    mov ecx, 65001 ; utf-8 code page
    xor edx, edx
    mov rdi, rax
    mov r8, rax
    mov [rsp + 40], rdx
    mov [rsp + 48], rdx
    mov [rsp + 56], rdx
    mov r9d, -1    ; string is null-terminated
    call WideCharToMultiByte
    mov esi, eax

    mov rcx, rdi
    lea rdx, [rsp + 64]
    call CommandLineToArgvW
    mov r12, rax
    mov r13, rax

    mov edx, esi
    xor ecx, ecx
    mov r8d, 0x3000 ; reseve and commit
    mov r9d, 0x04   ; read-write flags
    call VirtualAlloc  ; this is needed since the documentation states we can't reuse the space of ArgvW

    mov ebx, [rsp + 64]
    mov ecx, ebx
    mov rdi, rax
    shl ecx, 4
    sub rbp, rcx    ; allocate space for string references
    xor edx, edx
    lea rsp, [rbp - 64] ; allocate some more space for stack parameters
    mov [rsp + 40], rsi
    mov [rsp + 48], rdx
    mov [rsp + 56], rdx
    mov r14, rbx
    mov rsi, rbp

.L001:
        mov r8, [r12]
        mov ecx, 65001  ; utf-8 code page
        mov [rsp + 32], rdi
        mov r9d, -1     ; string is null-terminated
        call WideCharToMultiByte
        sub eax, 1  ; discard null terminator
        xor edx, edx
        mov [rbp], rdi
        mov [rbp + 8], rax
        add rdi, rax
        add rbp, 16
        add r12, 8
    sub ebx, 1
    jne .L001    
    
    mov rcx, r13    ; free the space used by ArgvW, as the documentation recommends
    call LocalFree

    mov rsp, rsi
    push r14
    push rsi
    mov rcx, rsp

    call main

.exit:
    mov ecx, eax
    call ExitProcess
    int3

align 16, int3
__main:
    ret
    int3
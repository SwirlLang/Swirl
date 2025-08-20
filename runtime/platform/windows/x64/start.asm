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

_start:
    sub rsp, 8
    mov rbp, rsp
    call GetCommandLineW

    mov rcx, rax
    lea rdx, [rsp + 64]
    call CommandLineToArgvW

    mov ebx, [rsp + 64]
    xor esi, esi
    mov rdi, rax
    mov r12, rax
    mov r13, rax
    mov [rsp + 40], rsi
    mov [rsp + 48], rsi
    mov [rsp + 56], rsi

.L000:
        mov r8, [rdi]
        xor edx, edx
        mov ecx, 65001
        mov r9d, -1
        call WideCharToMultiByte
        add rdi, 8
        add esi, eax
    sub ebx, 1
    jne .L000

    mov edx, esi
    xor ecx, ecx
    mov r8d, 0x3000
    mov r9d, 0x04
    call VirtualAlloc

    mov ebx, [rsp + 64]
    mov ecx, ebx
    mov rdi, rax
    shl ecx, 4
    sub rbp, rcx
    xor edx, edx
    lea rsp, [rbp - 80]
    mov [rsp + 40], rsi
    mov [rsp + 48], rdx
    mov [rsp + 56], rdx
    mov [rsp + 64], rbx
    mov rsi, rbp

.L001:
        mov r8, [r12]
        mov ecx, 65001
        mov r9d, -1
        mov [rsp + 32], rdi
        call WideCharToMultiByte
        sub eax, 1
        xor edx, edx
        mov [rbp], rdi
        mov [rbp + 8], rax
        add rdi, rax
        add rbp, 16
        add r12, 8
    sub ebx, 1
    jne .L001    
    
    mov rcx, r13
    call LocalFree

    mov ebx, [rsp + 64]
    mov rsp, rsi
    push rbx
    push rsi
    mov rcx, rsp

    call main

.exit:
    mov ecx, eax
    call ExitProcess
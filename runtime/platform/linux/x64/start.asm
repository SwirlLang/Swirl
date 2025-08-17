[BITS 64]

section .text
    extern main:function
    global _start:

_start:
    mov esi, [rsp]
    mov edx, esi
    shl edx, 4
    lea rbp, [rsp + 8]
    sub rsp, rdx
    lea rdi, [rbp - 24]
    mov rbx, rsp
    cmp esi, 1
    je short .L001

.L000:  ; calculate string sizes using pointer arithmetic
        mov rcx, [rbp]
        mov rdx, [rbp + 8]
        sub edx, ecx    ; this works since args are less than 2^32 bytes long. rcx is unmodified
        mov [rbx], rcx
        sub edx, 1
        mov [rbx + 8], rdx
    
    add rbp, 8
    add rbx, 16
    cmp rdi, rbx
    jne .L000

.L001: ; handle last string arg
    mov rdi, [rbp]
    mov ecx, -1
    lea rdx, [rdi + 1]
    repne scasb

    sub rdi, rdx
    sub rdx, 1
    mov [rbx + 8], rdi
    mov [rbx], rdx

    mov rdi, rsp
    mov rbp, rsp

    call main

.exit:
    mov edi, eax
    mov eax, 231
    syscall
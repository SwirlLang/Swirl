[BITS 32]

section .text
    extern main:function
    global _start:

_start:
    mov esi, [esp]
    mov edx, esi
    shl edx, 3
    lea ebp, [esp + 4]
    sub esp, edx
    lea edi, [ebp - 12]
    mov ebx, esp
    cmp esi, 1
    je short .L001

.L000:  ; calculate (argc - 1) string sizes using pointer arithmetic
        mov ecx, [ebp]
        mov edx, [ebp + 4]
        sub edx, ecx
        mov [ebx], ecx
        sub edx, 1
        mov [ebx + 4], edx
    
    add ebp, 4
    add ebx, 8
    cmp edi, ebx
    jne .L000

.L001: ; handle last string size
    mov edi, [ebp]
    mov ecx, -1
    lea edx, [edi + 1]
    repne scasb

    sub edi, edx
    sub edx, 1
    mov [ebx + 4], edi
    mov [ebx], edx

    mov ebp, esp
    mov edi, esp
    push esi
    push edi

    call main

.exit:
    mov ebx, eax
    mov eax, 252
    int 0x80
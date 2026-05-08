    ; ============================================
    ; MiniLang Compiler - x86-64 Assembly Output
    ; Target: Linux x86-64, System V AMD64 ABI
    ; Assembler: NASM (nasm -f elf64)
    ; ============================================

    section .text

    global main

main:
    push rbp    ; save base pointer
    mov rbp, rsp    ; set new base pointer
    ; leaf function using red zone

    mov qword [rbp-144], 10
    mov qword [rbp-152], 3
    mov rax, qword [rbp-144]    ; load left operand
    add rax, qword [rbp-152]    ; add
    mov qword [rbp-136], rax    ; save temp
    mov rax, qword [rbp-136]
    mov qword [rbp-160], rax
    mov rax, qword [rbp-144]    ; load left operand
    sub rax, qword [rbp-152]    ; sub
    mov qword [rbp-136], rax    ; save temp
    mov rax, qword [rbp-136]
    mov qword [rbp-168], rax
    mov rax, qword [rbp-144]    ; load left operand
    imul rax, qword [rbp-152]    ; mul
    mov qword [rbp-136], rax    ; save temp
    mov rax, qword [rbp-136]
    mov qword [rbp-176], rax
    mov rax, qword [rbp-144]
    xor rdx, rdx    ; clear rdx for div
    idiv qword [rbp-152]
    mov qword [rbp-136], rax    ; save temp
    mov rax, qword [rbp-136]
    mov qword [rbp-184], rax
    mov rax, qword [rbp-144]
    xor rdx, rdx    ; clear rdx for div
    idiv qword [rbp-152]
    mov rax, rdx    ; get remainder
    mov qword [rbp-136], rax    ; save temp
    mov rax, qword [rbp-136]
    mov qword [rbp-192], rax
    mov rax, qword [rbp-160]    ; load left operand
    add rax, qword [rbp-168]    ; add
    mov qword [rbp-136], rax    ; save temp
    mov rax, qword [rbp-136]    ; load left operand
    add rax, qword [rbp-176]    ; add
    mov qword [rbp-136], rax    ; save temp
    mov rax, qword [rbp-136]    ; load left operand
    add rax, qword [rbp-184]    ; add
    mov qword [rbp-136], rax    ; save temp
    mov rax, qword [rbp-136]    ; load left operand
    add rax, qword [rbp-192]    ; add
    mov qword [rbp-136], rax    ; save temp
    mov rax, qword [rbp-136]
    mov qword [rbp-200], rax
    mov rax, qword [rbp-200]
    jmp main_exit

main_exit:
    pop rbp    ; restore base pointer
    ret    ; return to caller


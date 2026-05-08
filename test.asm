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

    mov rax, 42
    jmp main_exit

main_exit:
    pop rbp    ; restore base pointer
    ret    ; return to caller


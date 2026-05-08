section .text
global main
extern printString
extern readInt
extern printInt
extern exit

main:
    push rbp
    mov rbp, rsp
    
    ; Prompt user
    lea rdi, [rel prompt]
    call printString
    
    ; Read integer
    call readInt
    
    ; Print result
    mov rdi, rax
    call printInt
    
    ; Exit
    mov rdi, 0
    call exit

section .data
prompt: db "Enter a number: ", 0

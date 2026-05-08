; ============================================
; MiniLang Runtime Library
; System V AMD64 ABI
; Linux x86-64
; ============================================

section .text

; Объявляем внешние символы
extern main

; ============================================
; printInt - Prints an integer to stdout
; Input: rdi = integer to print
; ============================================
global printInt
printInt:
    push rbp
    mov rbp, rsp
    sub rsp, 32                  ; Buffer for string conversion
    
    mov rax, rdi                 ; Number to convert
    mov rdi, rsp                 ; Buffer on stack
    call intToStr                ; Convert integer to string
    
    ; Calculate string length
    mov rsi, rsp
    xor rdx, rdx
    
.count_len:
    cmp byte [rsi + rdx], 0
    je .write
    inc rdx
    jmp .count_len
    
.write:
    ; Write to stdout (syscall 1)
    mov rax, 1                   ; syscall: write
    mov rdi, 1                   ; fd: stdout
    ; rsi already set to buffer
    ; rdx = length
    syscall
    
    ; Print newline
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel newline]
    mov rdx, 1
    syscall
    
    mov rsp, rbp
    pop rbp
    ret

; ============================================
; printString - Prints a null-terminated string
; Input: rdi = pointer to string
; ============================================
global printString
printString:
    push rbp
    mov rbp, rsp
    
    ; Calculate string length
    mov rsi, rdi                 ; String pointer
    xor rdx, rdx                 ; Counter
    
.count_loop:
    cmp byte [rsi + rdx], 0
    je .write
    inc rdx
    jmp .count_loop
    
.write:
    mov rax, 1                   ; syscall: write
    mov rdi, 1                   ; fd: stdout
    ; rsi already set
    ; rdx = length
    syscall
    
    pop rbp
    ret

; ============================================
; printChar - Prints a single character
; Input: rdi = character to print
; ============================================
global printChar
printChar:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    
    mov [rsp], dil               ; Store character on stack
    
    mov rax, 1                   ; syscall: write
    mov rdi, 1                   ; fd: stdout
    lea rsi, [rsp]               ; pointer to character
    mov rdx, 1                   ; length = 1
    syscall
    
    mov rsp, rbp
    pop rbp
    ret

; ============================================
; readInt - Reads an integer from stdin
; Returns: rax = integer value read
; ============================================
global readInt
readInt:
    push rbp
    mov rbp, rsp
    sub rsp, 32                  ; Buffer for input
    
    ; Read from stdin (syscall 0)
    mov rax, 0                   ; syscall: read
    mov rdi, 0                   ; fd: stdin
    mov rsi, rsp                 ; Buffer on stack
    mov rdx, 32                  ; Max bytes to read
    syscall
    
    ; Check for EOF or error
    cmp rax, 0
    jle .return_zero
    
    ; Convert string to integer
    mov rdi, rsp                 ; Buffer
    mov rdx, rax                 ; Bytes read
    call strToInt
    
    mov rsp, rbp
    pop rbp
    ret
    
.return_zero:
    xor rax, rax
    mov rsp, rbp
    pop rbp
    ret

; ============================================
; readString - Reads a string from stdin
; Input:  rdi = buffer pointer
;         rsi = buffer size
; Returns: rax = number of bytes read
; ============================================
global readString
readString:
    push rbp
    mov rbp, rsp
    
    ; Read from stdin (syscall 0)
    mov rax, 0                   ; syscall: read
    mov rdx, rsi                 ; buffer size
    mov rsi, rdi                 ; buffer pointer
    mov rdi, 0                   ; fd: stdin
    syscall
    
    ; Null-terminate the string (replace newline)
    cmp rax, 0
    jle .done
    cmp byte [rsi + rax - 1], 10 ; Check for newline
    jne .not_newline
    mov byte [rsi + rax - 1], 0  ; Replace newline with null
    dec rax                       ; Decrease length
    jmp .done
    
.not_newline:
    mov byte [rsi + rax], 0      ; Null-terminate
    
.done:
    pop rbp
    ret

; ============================================
; readChar - Reads a single character from stdin
; Returns: rax = character read
; ============================================
global readChar
readChar:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    
    ; Read from stdin (syscall 0)
    mov rax, 0                   ; syscall: read
    mov rdi, 0                   ; fd: stdin
    mov rsi, rsp                 ; Buffer on stack
    mov rdx, 1                   ; Read 1 byte
    syscall
    
    cmp rax, 1
    jne .return_zero
    
    movzx rax, byte [rsp]        ; Load character
    
    mov rsp, rbp
    pop rbp
    ret
    
.return_zero:
    xor rax, rax
    mov rsp, rbp
    pop rbp
    ret

; ============================================
; exit - Exits the program
; Input: rdi = exit code (0-255)
; ============================================
global exit
exit:
    mov rax, 60                  ; syscall: exit
    syscall
    ; Does not return

; ============================================
; _start - Program entry point
; Receives argc and argv from the OS
; Stack layout on entry:
;   [rsp]      = argc
;   [rsp+8]    = argv[0] (program name)
;   [rsp+16]   = argv[1] (first argument)
;   ...
;   [rsp+8*argc+8] = NULL
; ============================================
global _start
_start:
    ; Load argc into rdi (first argument to main)
    mov rdi, [rsp]
    
    ; Load argv into rsi (second argument to main)
    lea rsi, [rsp + 8]
    
    ; Optionally load envp into rdx (third argument)
    ; lea rdx, [rsp + 8 + rdi*8 + 8]
    
    ; Call main(argc, argv)
    call main
    
    ; Exit with main's return value
    mov rdi, rax
    call exit

; ============================================
; intToStr - Convert integer to string
; Input:  rax = integer
;         rdi = buffer (at least 21 bytes for 64-bit)
; Output: buffer filled with null-terminated string
; ============================================
intToStr:
    push rbx
    push rcx
    push rdx
    push r8                      ; Save sign flag
    
    mov rbx, rdi                 ; Save buffer pointer
    mov rcx, 0                   ; Digit counter
    mov r8, 0                    ; Sign flag (0 = positive)
    mov rdi, 10                  ; Divisor
    
    ; Handle zero case
    test rax, rax
    jnz .check_negative
    mov byte [rbx], '0'
    mov byte [rbx+1], 0
    jmp .done
    
.check_negative:
    ; Check if negative
    cmp rax, 0
    jge .convert_loop
    neg rax                      ; Make positive
    mov r8, 1                    ; Set sign flag
    
.convert_loop:
    xor rdx, rdx
    div rdi                      ; Divide rax by 10
    add dl, '0'                  ; Convert to ASCII
    push rdx                     ; Save digit on stack
    inc rcx                      ; Increment counter
    test rax, rax
    jnz .convert_loop
    
    ; Add minus sign if negative
    cmp r8, 0
    je .pop_digits
    mov byte [rbx], '-'
    inc rbx
    
.pop_digits:
    ; Pop digits to buffer
    mov rdx, 0
.store_loop:
    pop rax
    mov [rbx + rdx], al
    inc rdx
    loop .store_loop
    
    mov byte [rbx + rdx], 0     ; Null terminate
    
.done:
    pop r8
    pop rdx
    pop rcx
    pop rbx
    ret

; ============================================
; strToInt - Convert string to integer
; Input:  rdi = pointer to string
;         rdx = length of string
; Output: rax = integer value
; ============================================
strToInt:
    push rbx
    push rcx
    push rdx
    push r8
    
    xor rax, rax                 ; Result
    xor rcx, rcx                 ; Counter
    mov r8, 1                    ; Sign (1 = positive)
    
    ; Skip leading whitespace
.skip_whitespace:
    cmp rcx, rdx
    jge .done
    movzx r9, byte [rdi + rcx]
    cmp r9, ' '
    je .next_char
    cmp r9, 9                    ; Tab
    je .next_char
    cmp r9, 10                   ; Newline
    je .next_char
    cmp r9, 13                   ; Carriage return
    je .next_char
    jmp .check_sign
    
.next_char:
    inc rcx
    jmp .skip_whitespace
    
.check_sign:
    ; Check for minus sign
    movzx r9, byte [rdi + rcx]
    cmp r9, '-'
    jne .check_plus
    mov r8, -1                   ; Negative sign
    inc rcx
    jmp .convert
    
.check_plus:
    cmp r9, '+'
    jne .convert
    inc rcx
    
.convert:
    cmp rcx, rdx
    jge .apply_sign
    
    movzx r9, byte [rdi + rcx]
    
    ; Check if digit
    cmp r9, '0'
    jl .apply_sign
    cmp r9, '9'
    jg .apply_sign
    
    ; Multiply by 10 and add digit
    imul rax, 10
    sub r9, '0'
    add rax, r9
    
    inc rcx
    jmp .convert
    
.apply_sign:
    ; Apply sign
    imul rax, r8
    
.done:
    pop r8
    pop rdx
    pop rcx
    pop rbx
    ret

; ============================================
; strlen - Calculate string length
; Input:  rdi = pointer to null-terminated string
; Output: rax = length
; ============================================
global strlen
strlen:
    push rdi
    xor rax, rax
    
.loop:
    cmp byte [rdi], 0
    je .done
    inc rdi
    inc rax
    jmp .loop
    
.done:
    pop rdi
    ret

; ============================================
; strcmp - Compare two strings
; Input:  rdi = first string
;         rsi = second string
; Output: rax = 0 if equal, <0 if s1<s2, >0 if s1>s2
; ============================================
global strcmp
strcmp:
    push rdi
    push rsi
    
.loop:
    mov al, [rdi]
    mov bl, [rsi]
    
    cmp al, bl
    jne .not_equal
    cmp al, 0
    je .equal
    
    inc rdi
    inc rsi
    jmp .loop
    
.not_equal:
    movzx rax, al
    movzx rbx, bl
    sub rax, rbx
    jmp .done
    
.equal:
    xor rax, rax
    
.done:
    pop rsi
    pop rdi
    ret

; ============================================
; strcpy - Copy string
; Input:  rdi = destination buffer
;         rsi = source string
; Output: rdi = destination buffer
; ============================================
global strcpy
strcpy:
    push rdi
    
.loop:
    mov al, [rsi]
    mov [rdi], al
    cmp al, 0
    je .done
    inc rdi
    inc rsi
    jmp .loop
    
.done:
    pop rax
    ret

; ============================================
; memset - Fill memory with a byte value
; Input:  rdi = pointer to memory
;         rsi = byte value
;         rdx = number of bytes
; Output: rax = original pointer
; ============================================
global memset
memset:
    push rdi
    mov rcx, rdx
    mov al, sil
    rep stosb
    pop rax
    ret

; ============================================
; memcpy - Copy memory
; Input:  rdi = destination
;         rsi = source
;         rdx = number of bytes
; Output: rax = destination
; ============================================
global memcpy
memcpy:
    push rdi
    mov rcx, rdx
    rep movsb
    pop rax
    ret

; ============================================
; Data section
; ============================================
section .data
newline: db 10
space:   db 32
tab:     db 9

; ============================================
; Stack canary note for security
; ============================================
section .note.GNU-stack noalloc noexec nowrite progbits
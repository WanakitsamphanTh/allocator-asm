%include "src/common.mac"
section .rodata
    no_mem_str db "no memory left", 10, 0
    printf_fmt db "     allocated %d at %p", 10, 0
    bar_fmt db "============================================", 10, 0

section .bss
    ptr:
        .0 resq 1
        .1 resq 1
        .2 resq 1
section .text
    extern alloc
    extern release
    extern printf

global main
main:
    push rbp
    mov rbp, rsp

    mov rdi, 8
    call alloc
    mov qword [rel ptr.0], rax

    lea rdi, [rel printf_fmt]
    mov rsi, 8
    mov rdx, qword [rel ptr.0]
    xor eax, eax
    call printf

    mov rdi, 200
    call alloc
    mov qword [rel ptr.1], rax

    lea rdi, [rel printf_fmt]
    mov rsi, 200
    mov rdx, qword [rel ptr.1]
    xor rax, rax
    call printf

    mov rdi, 148
    call alloc
    mov qword [rel ptr.2], rax

    lea rdi, [rel printf_fmt]
    mov rsi, 148
    mov rdx, qword [rel ptr.2]
    xor rax, rax
    call printf

    mov rdi, qword [rel ptr.0]
    call release

    mov rdi, qword [rel ptr.1]
    call release

    mov rdi, qword [rel ptr.2]
    call release

    lea rdi, [rel bar_fmt]
    xor eax, eax
    call printf

    mov rdi, 32
    call alloc
    mov qword [rel ptr.0], rax

    lea rdi, [rel printf_fmt]
    mov rsi, 32
    mov rdx, qword [rel ptr.0]
    xor rax, rax
    call printf

    mov rdi, 8
    call alloc
    mov qword [rel ptr.1], rax

    lea rdi, [rel printf_fmt]
    mov rsi, 8
    mov rdx, qword [rel ptr.1]
    xor rax, rax
    call printf

    mov rdi, 100
    call alloc
    mov qword [rel ptr.2], rax

    lea rdi, [rel printf_fmt]
    mov rsi, 100
    mov rdx, qword [rel ptr.2]
    xor rax, rax
    call printf

    mov rdi, qword [rel ptr.0]
    call release

    mov rdi, qword [rel ptr.1]
    call release

    mov rdi, qword [rel ptr.2]
    call release

    leave
    ret
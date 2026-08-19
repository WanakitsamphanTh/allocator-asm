%include "src/common.mac"
section .rodata
    no_mem_str db "no memory left", 10, 0
    print_fmt db "[%05llu] allocated %d at %p", 10, 0
    block_size equ 8
section .data
section .text
    extern alloc
    extern free
    extern printf

global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 80
    push rbx
    xor rbx, rbx
    push r15
    mov r15, block_size
.loop:
    cmp rbx, 10
    jae .done
    mov rdi, r15
    call alloc
    test rax, rax
    jz .no_mem
    mov r9, rbp
    mov r10, rbx
    imul r10, 8
    sub r9, r10
    mov qword [r9], rax

    lea rdi, [rel print_fmt]
    mov rsi, rbx
    mov rdx, 15
    mov rcx, rax
    xor eax, eax
    libcall printf
    
    inc rbx
    add r15, block_size
    jmp .loop
.no_mem:
    lea rdi, [rel no_mem_str]
    xor eax, eax
    libcall printf
    pop rbx
.done:
    xor rbx, rbx
.free_loop:
    cmp rbx, 10
    jae .return

    mov r9, rbp
    mov r10, rbx
    imul r10, 8
    sub r9, r10
    mov rdi, qword [r9]
    call free

    inc rbx
    jmp .free_loop
.return:
    pop r15
    leave
    ret
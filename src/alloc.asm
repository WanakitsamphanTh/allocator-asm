%include "src/mem.mac"
%include "src/structure.mac"
%include "src/common.mac"

section .rodata
    
    %ifdef DEBUG
    init_report_fmt db "allocated new memory of size %llu", 10, "starting at %p", 10, "end at %p", 10, 0
    header_fmt db "%s : header=%llx prev_size = %d size = %d total %llu", 10, 0
    free_report_fmt db "free ptr=%p size = %d total %llu", 10, 0

    search_str db "search found ", 0
    bump_str db "bumped ", 0
    %endif

    mem_pool_size equ (1 << 30) ; 2^30 

section .bss
    tape resq 1
    brk resq 1
    end resq 1

    prev_size resd 1
    total_size resd 1

    bin_count equ 256
    largest_bin equ 16 * bin_count
    free_list resq bin_count  ; free_list of size 16 32 48 ... 16*bin_count 
    
    free_large:
        .by_addr resq 1
        .by_size resq 1

section .init_array
    dq init_alloc
section .fini_array
    dq fini_alloc

section .text
    extern printf
    extern tree_add
    extern tree_del
    extern mem_tree_init
    extern cmp_by_addr
    extern cmp_by_size
    extern cmp_with_size
    extern tree_traverse

global alloc
alloc:
    ; rbx := size
    ; r12 := ptr
    push rbp
    mov rbp, rsp
    push rbx
    mov rbx, rdi
    push r12
    test rbx, rbx
    jz .return

    ; adjust size to be 16-byte aligned
    movzx rbx, ebx
    add ebx, header_t_size
    add ebx, 15
    and ebx, -16

    ; search_fit
    mov rdi, rbx
    call search_fit
    test rax, rax
    %ifdef DEBUG
    mov r12, rax
    lea rsi, [rel search_str]
    jnz .report
    %else
    jnz .return
    %endif

    ; bump
    mov r12, qword [rel brk]
    add r12, rbx
    mov rcx, qword [rel end]
    cmp r12, rcx
    jae .null
    mov qword [rel brk], r12
    sub r12, rbx
    add r12, header_t_size

    ; update header
    movzx rcx, dword [rel prev_size]
    header_set_flags r12, rbx, rcx, OCCUPIED

    ; update prev_size
    mov dword [rel prev_size], ebx

    ; update total
    mov ecx, dword [rel total_size]
    add ecx, ebx
    mov dword [rel total_size], ecx

%ifdef DEBUG
    lea rsi, [rel bump_str]
.report:
    lea rdi, [rel header_fmt]
    header_get_flags rdx, r12
    header_get_prevsize rcx, r12
    header_get_size r8, r12
    movzx r9, dword [rel total_size]
    xor eax, eax
    libcall printf
%endif

    mov rax, r12
.return:
    pop r12
    pop rbx
    leave
    ret
.null:
    mov rax, 0
    jmp .return

global free
free:
    ; rbx := ptr to free
    push rbp
    mov rbp, rsp
    push rbx
    mov rbx, rdi

    header_get_flags rax, rbx    ; rax := rbx->header.size
    mov rcx, 0x7fffffffffffffff
    and rax, rcx 
    mov qword [rbx - header_t_size], rax
    movzx rax, eax
    mov ecx, dword [rel total_size]
    sub ecx, eax
    mov dword [rel total_size], ecx
    cmp rax, largest_bin
    ja .large

    mov rcx, 16
    div rcx 
    dec rax                     ; rax := index = size / 16 - 1
    cmp rax, 255
    jae .large

    lea rcx, [rel  free_list]
    lea rcx, [rcx + rax * 8]    ; rcx := &free_list[rax]
    mov rax, qword [rcx]        ; rax := *rcx = free_list[rax]
    list_push_front rax, rbx
    mov qword [rcx], rax        ; *rcx = rdi
    jmp .then
    
.large:
    mov rdi, rbx
    call mem_tree_init

    mov rdi, qword [rel free_large.by_addr]
    lea rsi, [rbx + mem_tree_t.by_addr]
    lea rdx, [rel cmp_by_addr]
    call tree_add
    mov qword [rel free_large.by_addr], rax
    
    mov rdi, qword [rel free_large.by_size]
    lea rsi, [rbx + mem_tree_t.by_size]
    lea rdx, [rel cmp_by_size]
    call tree_add
    mov qword [rel free_large.by_size], rax

.then:
    %ifdef DEBUG
    mov rsi, rdi
    header_get_size rdx, rdi
    movzx rcx, dword [rel total_size]
    lea rdi, [rel free_report_fmt]
    xor eax, eax
    libcall printf
    %endif

    pop rbx
    leave
    ret

; ==================== internal ====================

search_fit:
    xor rax, rax
    ret

; ==================== init/fini ====================

init_alloc:
    endbr64
    mov rax, 9  ; sys_mmap
    mov rdi, 0  ; addr
    mov rsi, mem_pool_size ; size
    mov rdx, 0x03   ; PROT_WRITE (0x02) | PROT_READ (0x01)
    mov r10, 0x22   ; MAP_PRIVATE (0x02) | MAP_ANONYMOUS (0x20)
    mov r8, -1
    mov r9, 0
    syscall
    test rax, rax
    jz .terminate
    mov qword [rel tape], rax
    mov qword [rel brk], rax
    add rax, mem_pool_size
    mov qword [rel end], rax
    %ifdef DEBUG
    lea rdi, [rel init_report_fmt]
    mov rsi, mem_pool_size
    mov rdx, qword [rel tape]
    mov rcx, qword [rel end]
    libcall printf
    %endif
    ret
.terminate:
    mov rax, 60
    mov rdi, -1
    syscall
fini_alloc:
    endbr64
    mov rax, 11 ; munmap
    mov rdi, qword [rel tape]
    mov rsi, mem_pool_size
    syscall
    ret

%include "src/common.mac"
%include "src/mem.mac"
%include "src/structure.mac"

; typedef int64_t (*cmpfn)(tree_node_t* a,ptr* b);
; returns
;       rax > 0 if a > b
;       rax = 0 if a = b
;       rax < 0 if a < b
; typedef tree_t tree_node_t

section .rodata
    tree_del_table:
        dq tree_del.c_1
        dq tree_del.c_2
        dq tree_del.c_3
        dq tree_del.c_4

section .text

global tree_add ; tree_t* tree_add(tree_node_t*, ptr*, cmpfn)
tree_add:
    test rdi, rdi
    jz .new_root
    ; rbx := root, r12 := ptr, r13 := cmpfn
    push rbp
    mov rbp, rsp
    push rbx
    mov rbx, rdi
    push r12
    mov r12, rsi
    push r13

    mov rdi, rbx
    mov rsi, r12
    call r13
    test rax, rax
    jz .return
    js .lt
    ; cur > new : to right
    ; cur < new : to left
    ; cur = new (should be impossible) : return
.gt:
    tree_get_right rdi, rbx
    test rdi, rdi
    jnz .gt_rec
    tree_set_right rbx, r12
    jmp .return
.gt_rec:
    mov rsi, r12
    mov rdx, r13
    call tree_add
    tree_set_right rbx, rax
    jmp .return    
.lt:
    tree_get_left rdi, rbx
    test rdi, rdi
    jnz .gt_rec
    tree_set_left rbx, r12
    jmp .return
.lt_rec:
    mov rsi, r12
    mov rdx, r13
    call tree_add
    tree_set_left rbx, rax
.return:
    mov rax, rbx
    pop r13
    pop r12
    pop rbx
    leave
    ret
.new_root:
    mov rax, rdi
    ret

global tree_del ; tree_t* tree_del(tree_t*, tree_t*, cmpfn)
tree_del:
    ; rbx := root, r12 := node to delete, r13 := cmpfn
    test rdi, rdi
    jz .null

    push rbp
    mov rbp, rsp
    push rbx
    mov rbx, rdi
    push r12
    mov r12, rsi
    push r13
    mov r13, rdx

    ; compare node ('s value) with value
    mov rdi, rbx
    mov rsi, r12
    call r13
    test rax, rax
    js .del_left    ; < 0
    jnz .del_right  ; > 0

    ; delete this node
    ; case 1: no children (!l && !r)
    ; case 2: one child: l (l && !r)
    ; case 3: one child: r (!l && r)
    ; case 4: two children (l && r)

    tree_get_left rcx, rbx 
    tree_get_right rdx, rbx
    xor rax, rax
    test rcx, rcx
    setnz al         ; al := left == NULL
    test rdx, rdx
    setnz bl         ; bl := right == NULL
    shl bl, 1
    or al, bl       ; al[0] = left == NULL, al[1] = right == NULL
    jmp qword [tree_del_table + rax]

    ; rcx = left, rdx = right
.c_1:
    mov rax, 0
    jmp .return
.c_2:
    mov rdi, rcx
    mov rsi, r12
    mov rdx, r13
    call tree_del
    tree_set_left rbx, rax
    mov rax, rbx
    jmp .return
.c_3:
    mov rdi, rdx
    mov rsi, r12
    mov rdx, r13
    call tree_del
    tree_set_right rbx, rax
    mov rax, rbx
    jmp .return
.c_4:
    ; to do
    jmp .return

.del_left:
    tree_get_left rdi, rbx
    mov rsi, r12
    mov rdx, r13
    call tree_del
    mov qword [rbx + tree_t.left], rax
    mov rax, rbx
    jmp .return
.del_right:
    tree_get_left rdi, rbx
    mov rsi, r12
    mov rdx, r13
    call tree_del
    mov qword [rbx + tree_t.left], rax
    mov rax, rbx
.return:
    pop r13
    pop r12
    pop rbx
    leave
    ret
.null:
    xor rax, rax
    ret

global tree_traverse
tree_traverse:
    ; rbx := root, r12 := val to delete, r13 := cmpfn
    push rbp
    mov rbp, rsp
    push rbx
    mov rbx, rdi
    push r12
    mov r12, rsi
    push r13
    mov r13, rdx
.1:
    test rbx, rbx
    xor rax, rax
    jz .return

    mov rdi, rbx
    mov rsi, r12
    call r13

    test rax, rax
    js .l
    jnz .r
    mov rax, rbx
.return:
    pop r13
    pop r12
    pop rbx
    leave
    ret
.l:
    tree_get_left rbx
    jmp .1
.r:
    tree_get_right rbx
    jmp .1

%macro inlined_tree_init 1
    mov qword [%1 + tree_t.left], 0
    mov qword [%1 + tree_t.right], 0
%endmacro

global tree_init
tree_init: ; void tree_init(tree_t);
    inlined_tree_init rdi
    ret

global mem_tree_init
mem_tree_init:
    lea rsi, [rdi + mem_tree_t.by_addr]
    inlined_tree_init rsi
    lea rsi, [rdi + mem_tree_t.by_size]
    inlined_tree_init rsi
    ret

; ============== internal functions ==============


; ============== comparison ==============

global cmp_by_addr
cmp_by_addr:
    endbr64
    container_of rdi, rdi, mem_tree_t.by_addr
    mov rax, rdi
    container_of rsi, rsi, mem_tree_t.by_addr
    sub rax, rsi
    ret

global cmp_by_size
cmp_by_size:
    endbr64
    container_of rax, rdi, mem_tree_t.by_size
    header_get_size rax, rax
    container_of rdx, rsi, mem_tree_t.by_size
    header_get_size rdx, rdx
    sub rax, rdx
    test rax, rax
    jz cmp_by_addr
    ret

global cmp_with_size
cmp_with_size:
    endbr64
    container_of rdi, rdi, mem_tree_t.by_size
    header_get_size rax, rdi
    sub rax, rsi
    ret
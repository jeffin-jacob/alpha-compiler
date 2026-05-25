.L0:
fun1:
    pushq %rbp
    movq %rsp, %rbp
    movq %rdi, 16(%rbp)
    movq %rsi, 24(%rbp)
    subq $11, %rsp
    movl 16(%rbp), %rbx
    movl 24(%rbp), %rcx
    movq %rbx, %rsi
    cmpb %rcx, %rbx
    movq $0, %rbx
    movq $1, %rax
    cmove %rax, %rbx
    movb %rbx, -10(%rbp)
    movb -10(%rbp), %rbx
    cmpb $1, %rbx
    je .L4
.L3:
    jmp .L6
.L4:
    movq $1, %rbx
    movb %rbx, -9(%rbp)
    jmp .L7
.L6:
    movq $0, %rbx
    movb %rbx, -9(%rbp)
.L7:
    movb -9(%rbp), %rbx
    cmpb $1, %rbx
    je .L9
.L8:
    jmp .L11
.L9:
    movq $1, %rbx
    movb %rbx, -11(%rbp)
    jmp .L12
.L11:
    movq $0, %rbx
    movb %rbx, -11(%rbp)
.L12:
    movq %rbx, %rax
    movq %rbp, %rsp
    popq %rbp
    ret
.L13:
fun2:
    pushq %rbp
    movq %rsp, %rbp
    movq %rdi, 16(%rbp)
    subq $3, %rsp
    movq %rbx, %rdi
    movq %rbx, %rsi
    subq $16, %rsp
    call fun1
    movq %rax, %rbx
.L17:
    cmpb $1, %rbx
    je .L19
.L18:
    jmp .L21
.L19:
    movq $1, %rbx
    movb %rbx, -1(%rbp)
    jmp .L22
.L21:
    movq $0, %rbx
    movb %rbx, -1(%rbp)
.L22:
    movb -1(%rbp), %rbx
    cmpb $1, %rbx
    je .L24
.L23:
    jmp .L26
.L24:
    movq $1, %rbx
    movb %rbx, -3(%rbp)
    jmp .L27
.L26:
    movq $0, %rbx
    movb %rbx, -3(%rbp)
.L27:
    movq %rbx, %rax
    movq %rbp, %rsp
    popq %rbp
    ret
.L28:
    nop

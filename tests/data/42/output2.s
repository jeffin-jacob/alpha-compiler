.L0:
fun1:
    pushq %rbp
    movq %rsp, %rbp
    movq %rdi, 16(%rbp)
    movq %rsi, 24(%rbp)
    movq %rdx, 32(%rbp)
    subq $23, %rsp
    movl 16(%rbp), %rbx
    movl 24(%rbp), %rcx
    movq %rbx, %rsi
    cmpb %rcx, %rbx
    movq $0, %rbx
    movq $1, %rax
    cmove %rax, %rbx
    movb %rbx, -22(%rbp)
    movb -22(%rbp), %rbx
    cmpb $1, %rbx
    je .L4
.L3:
    jmp .L9
.L4:
    jmp .L5
.L5:
    movq $1, %rbx
    movb %rbx, -21(%rbp)
    jmp .L8
.L7:
    movq $0, %rbx
    movb %rbx, -21(%rbp)
.L8:
    jmp .L21
.L9:
    movl 32(%rbp), %rbx
    movq %rsi, %rcx
    cmpb %rbx, %rsi
    movq $0, %rsi
    movq $1, %rax
    cmove %rax, %rsi
    movb %rsi, -23(%rbp)
    movb -23(%rbp), %rbx
    cmpb $1, %rbx
    je .L12
.L11:
    jmp .L17
.L12:
    jmp .L15
.L13:
    movq $1, %rbx
    movb %rbx, -21(%rbp)
    jmp .L16
.L15:
    movq $0, %rbx
    movb %rbx, -21(%rbp)
.L16:
    jmp .L21
.L17:
    jmp .L18
.L18:
    movq $1, %rbx
    movb %rbx, -21(%rbp)
    jmp .L21
.L20:
    movq $0, %rbx
    movb %rbx, -21(%rbp)
.L21:
    movb -21(%rbp), %rbx
    cmpb $1, %rbx
    je .L23
.L22:
    jmp .L25
.L23:
    movq $1, %rbx
    movb %rbx, -23(%rbp)
    jmp .L26
.L25:
    movq $0, %rbx
    movb %rbx, -23(%rbp)
.L26:
    movq %rbx, %rax
    movq %rbp, %rsp
    popq %rbp
    ret
.L27:
fun2:
    pushq %rbp
    movq %rsp, %rbp
    movq %rdi, 16(%rbp)
    movq %rsi, 24(%rbp)
    subq $3, %rsp
    movq %rbx, %rdi
    movq %rbx, %rsi
    movq %rbx, %rdx
    subq $32, %rsp
    call fun1
    movq %rax, %rbx
.L32:
    cmpb $1, %rbx
    je .L34
.L33:
    jmp .L36
.L34:
    movq $1, %rbx
    movb %rbx, -1(%rbp)
    jmp .L37
.L36:
    movq $0, %rbx
    movb %rbx, -1(%rbp)
.L37:
    movb -1(%rbp), %rbx
    cmpb $1, %rbx
    je .L39
.L38:
    jmp .L41
.L39:
    movq $1, %rbx
    movb %rbx, -3(%rbp)
    jmp .L42
.L41:
    movq $0, %rbx
    movb %rbx, -3(%rbp)
.L42:
    movq %rbx, %rax
    movq %rbp, %rsp
    popq %rbp
    ret
.L43:
    nop

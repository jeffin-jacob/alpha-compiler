	.file	"alpha_lib.c"
	.section	.rodata
.LC0:
	.string	"%d"

	.text
	.globl	printInteger
	.type	printInteger, @function
printInteger:
.LFB2:
	pushq	%rbp			# push old base pointer
	movq	%rsp, %rbp		# move base pointer

	subq	$16, %rsp		# make room on stack
	movl	%edi, -4(%rbp)		# spill arg to stack
	movl	-4(%rbp), %eax		# move argument to %eax
	movl	%eax, %esi
	movl	$.LC0, %edi
	movl	$0, %eax
	call	printf
	movl	$0, %eax
	leave
	ret
.LFE2:
	.size	printInteger, .-printInteger

	.globl	printCharacter
	.type	printCharacter, @function
printCharacter:
.LFB4:
	pushq	%rbp
	movq	%rsp, %rbp

	subq	$16, %rsp
	movb	%dil, %al
	movb	%al, -4(%rbp)
	movsbl	-4(%rbp), %eax
	movl	%eax, %edi
	call	putchar
	movl	$0, %eax
	leave
	ret
.LFE4:
	.size	printCharacter, .-printCharacter

	.section	.rodata
.LC2:
	.string	"false"
.LC3:
	.string	"true"
.LC4:
	.string	"%s"

	.text
	.globl	printBoolean
	.type	printBoolean, @function
printBoolean:
.LFB5:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movb	%dil, -4(%rbp)
	cmpb	$0, -4(%rbp)
	jne	.L8
	movl	$.LC2, %eax
	jmp	.L9
.L8:
	movl	$.LC3, %eax
.L9:
	movq	%rax, %rsi
	movl	$.LC4, %edi
	movl	$0, %eax
	call	printf
	movl	$0, %eax
	leave
	ret
.LFE5:
	.size	printBoolean, .-printBoolean

	.globl	reserve
	.type	reserve, @function
reserve:
.LFB8:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movl	-4(%rbp), %eax
	cltq
	movq	%rax, %rdi
	call	malloc
	leave
	ret
.LFE8:
	.size	reserve, .-reserve

	.globl	release
	.type	release, @function
release:
.LFB9:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, %rdi
	call	free
	movl	$0, %eax
	leave
	ret
.LFE9:
	.size	release, .-release

	.ident	"GCC: (GNU) 6.4.0"
	.section	.note.GNU-stack,"",@progbits

#include <stdio.h>
#include "ir.h"

// a single value used in an instruction
typedef struct AsmOp {
    enum {
        ASM_OP_REG,    // a register stored in 'val'
        ASM_OP_IMM,    // an immediate constant in 'val' (int or bool/char)
        ASM_OP_OFFSET,   // a register w/ offset for memory location
        ASM_OP_LABEL,   // label stored in 'val' since we're finding leaders for basic blocks (compromises all possible jump targets) we can just shove a label on each of those w/ the ir array index
        ASM_OP_FUNC_LABEL, // pointer to symEntry stored in func_entry
        ASM_OP_NONE    // no operand
    } kind;
    int val;
    int offset;
    int ss;
    /* for functions */
    symEntry *func_entry;
} AsmOp;

typedef enum {
    ASM_END,          // indicate end of program
    ASM_MOV,          // dst = src
    ASM_LEAQ_FUNC,
    ASM_ADD,          // dst = dst + src
    ASM_SUB,          // dst = dst - src
    ASM_IMUL,         // dst = dst * src
    ASM_IDIV,         // eax = quotient, edx = remainder for edx:eax/src
                        // don't need separate remainder instr
    ASM_CLTD,         // sign extends eax into edx:eax, only needed before idiv
    ASM_NEG,          // dst = -dst
    ASM_NOT,          // dst = ~dst
    ASM_AND,          // dst = dst & src
    ASM_OR,           // dst = dst | src
    ASM_CMP,          // a, b     b-a set flags (can use for less than)
    ASM_JMP,          // unconditional jump to label dst
    ASM_JL,           // jump to label dst if cmp gave less than
    ASM_JE,           // jump to label dst if cmp gave equal to
    ASM_PUSH,         // push src
    ASM_POP,          // pop to dst
    ASM_CALL,         // push instruction ptr and jump to func label dst
    ASM_CALL_INDIRECT,
    ASM_RET,          // pop instruction ptr
    ASM_LABEL,        // label, in this case 'dst' gives ir array line # to print
    ASM_CMOVE,        // move src into dst if condition flag for 'equal' set
    ASM_CMOVL,         // move src into dst if condition flag for 'less than' set
    ASM_NOP         // no operation, useful for placeholders or if jump to 'end'
} AsmOpCode;

// B=byte, L=long(32bit), Q=quad(64bit)
#define B 2
#define L 1
#define Q 0

typedef struct AsmInstr {
    int ss;
    AsmOpCode op;
    AsmOp dst;
    AsmOp src;
} AsmInstr;

extern AsmInstr asm_array[];

//emission functions
void asm_emit(AsmInstr a);
void asm_emit_end();
void asm_emit_leaq_func(AsmOp dst, symEntry *func);
void asm_emit_mov_ss(AsmOp dst, AsmOp src, int ss);
void asm_emit_mov(AsmOp dst, AsmOp src);
void asm_emit_add(AsmOp dst, AsmOp src);
void asm_emit_addQ(AsmOp dst, AsmOp src);
void asm_emit_sub(AsmOp dst, AsmOp src);
void asm_emit_sub_ss(AsmOp dst, AsmOp src, int ss);
void asm_emit_add_ss(AsmOp dst, AsmOp src, int ss);
void asm_emit_imul(AsmOp dst, AsmOp src);
void asm_emit_idiv(AsmOp src);
void asm_emit_cltd();
void asm_emit_neg(AsmOp dst);
void asm_emit_not(AsmOp dst);
void asm_emit_and(AsmOp dst, AsmOp src);
void asm_emit_or(AsmOp dst, AsmOp src);
void asm_emit_cmp(AsmOp dst, AsmOp src);
void asm_emit_cmp_ss(AsmOp dst, AsmOp src, int ss);
void asm_emit_jmp(AsmOp dst);
void asm_emit_jl(AsmOp dst);
void asm_emit_je(AsmOp dst);
void asm_emit_push(AsmOp src);
void asm_emit_pop(AsmOp dst);
void asm_emit_call(AsmOp dst);
void asm_emit_call_indirect(AsmOp dst);
void asm_emit_ret();
void asm_emit_label(AsmOp dst);
void asm_emit_cmove(AsmOp dst, AsmOp src);
void asm_emit_cmovl(AsmOp dst, AsmOp src);
void asm_emit_nop();
// symEntrys have the following values for compiler backend.
// The offset and regNum together compose the address descriptor since each variable
// is in an address and possibly at most 1 register at a time
// {
// int regNum; Which register descriptor entry this is a part of if any, -1 means not in register
// bool alive;  Liveness value
// int nextUse; Which instruction in ir array is it next used, -1 means outside of current block
// }

//register descriptor should store a linkedlist of symentrys that are assigned
//to that register for each register in an array of 12 (would be 16 but
//skipping over rbp rsp since those are important for stack
//and skipping over rax/rdx to save headaches during division)
//Dont need to specify the size of registers like rbx vs ebx vs bx etc
//since we can use the instruction size suffixes movl vs movq etc for that
typedef struct registerDescriptorEntry registerDescriptorEntry;

struct registerDescriptorEntry { //need another indirection here since symEntry's own
    symEntry* cur;               //next pointer points to in symbol table
    registerDescriptorEntry* next;
};

extern registerDescriptorEntry registerDescriptor[]; //pointer to symentry for each reg

extern IrInstr ir_array[]; //the ir array

//when implementing this you gotta be careful. the book describes how to implement get_reg for
//instructions as x = y op z which would work in a normal assembly language but since x86 is evil
//the result of an operation is stored by default in the arg1 operand like dst = dst-src
//this means theres the question of whether y should be stored in memory or not since it
//always gets overwritten. there's more to be said here but im getting tired boss
typedef struct reg3 {
    int result;
    int arg1;
    int arg2;
} reg3;

typedef struct reg2 {
    int result;
    int arg1;
} reg2;

reg3 get_reg3(IrInstr i);
reg2 get_reg2(IrInstr i);
void registerContentDump();
char* asm_op_as_str(AsmOp asmop);
void asm_array_print(FILE* printloc);
int codegen(FILE* cg_file); //writes code to .s file

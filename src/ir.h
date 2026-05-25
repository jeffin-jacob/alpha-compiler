#ifndef IR_H
#define IR_H

#include <stdio.h>
#include "symbol-table.h"
#include <stdbool.h>

//BPList is a LL of indices to backpatch to later
typedef struct BPList BPList;
struct BPList {
    int idx;
    BPList* next;
};

BPList* newBPList(int i);
void addBPElem(BPList* head, int i);
BPList* mergeBPLists(BPList* i1, BPList* i2);
void backpatch(BPList* bp_list, int idx);
// all the possible IR operations
typedef enum {
    IR_NOTHING,      // no op
    IR_ASSIGN,       // x = y
    IR_ADD,          // x = y + z
    IR_SUB,          // x = y - z
    IR_MUL,          // x = y * z
    IR_DIV,          // x = y / z
    IR_REM,          // x = y % z
    IR_NEG,          // x = -y
    IR_NOT,          // x = !y
    IR_AND,          // x = y & z
    IR_OR,           // x = y | z
    IR_LT,           // x = y < z
    IR_EQ,           // x = y == z
    IR_GOTO,         // unconditional jump
    IR_IF_TRUE,      // jump to label x if bool y is true
    IR_FUNC_ADDR_ASSIGN,
    IR_CALL_INDIRECT,
    IR_IF_FALSE,     // jump if label x if bool y is false
    IR_FUNC_START,   // indicates function start pos in array (mainly for printing IR array)
    IR_PARAM,        // push argument before call
    IR_CALL,         // call a function x = y() (z is param count)
    IR_RETURN,       // return from function
    IR_ARRAY_ACCESS, // read from array
    IR_ARRAY_ASSIGN, // write to array
    IR_SEG_FAULT
} IrOpCode;

// a single value used in an instruction
typedef struct IrOp {
    enum {
        IR_OP_SYMENTRY,   // variable or function name
        IR_OP_INT,    // integer constant
        IR_OP_BOOL,   // boolean constant
        IR_OP_CHAR,   // character constant
        IR_OP_IR_ARRAY_IDX,  // ir array entry for jump targets etc
        IR_OP_PLACEHOLDER_ARRAY_IDX, // to indicate locations to be backpatched
        IR_OP_PARAM_CT,
        IR_OP_NONE,     // no operand
    } kind;
    union {
        symEntry* sym_entry;
        int int_val;
        int ir_array_idx;
        int param_ct;
        bool bool_val;
        char char_val;
    } val;
} IrOp;

typedef struct LNU {
    bool alive;
    int next_use;
} LNU;
// one three-address instruction: result = arg1 op arg2
typedef struct IrInstr {
    IrOpCode op;
    IrOp result;
    IrOp arg1;
    IrOp arg2;

    //The following stuff is only used in the backend
    bool is_leader;
    LNU resultLNU;
    LNU arg1LNU;
    LNU arg2LNU;
    
} IrInstr;

extern IrInstr ir_array[];
extern int ir_array_idx;

// ir operand generation functions
IrOp ir_op_from_symEntry(symEntry* e); 
IrOp ir_op_from_int(int i);
IrOp ir_op_from_bool(bool b);
IrOp ir_op_from_char(char c);
IrOp ir_op_from_ir_array_entry(int i);

// ir emission functions
void ir_emit_end();
void ir_emit_assign(IrOp result, IrOp arg1);
void ir_emit_func_addr_assign(IrOp result, IrOp fn_symEntry);
void ir_emit_add(IrOp result, IrOp arg1, IrOp arg2);
void ir_emit_sub(IrOp result, IrOp arg1, IrOp arg2);
void ir_emit_mul(IrOp result, IrOp arg1, IrOp arg2);
void ir_emit_div(IrOp result, IrOp arg1, IrOp arg2);
void ir_emit_rem(IrOp result, IrOp arg1, IrOp arg2);
void ir_emit_neg(IrOp result, IrOp arg1);
void ir_emit_not(IrOp result, IrOp arg1);
void ir_emit_and(IrOp result, IrOp arg1, IrOp arg2);
void ir_emit_or(IrOp result, IrOp arg1, IrOp arg2);
void ir_emit_lt(IrOp result, IrOp arg1, IrOp arg2);
void ir_emit_eq(IrOp result, IrOp arg1, IrOp arg2);
void ir_emit_goto(IrOp target);
void ir_emit_if_true(IrOp target, IrOp arg1);
void ir_emit_if_false(IrOp target, IrOp arg1);
//*_bp have no target and will be backpatched
void ir_emit_goto_bp();
void ir_emit_if_true_bp(IrOp arg1);
void ir_emit_if_false_bp(IrOp arg1);

void ir_emit_func_start(IrOp f_name);
void ir_emit_param(IrOp param);
void ir_emit_call(IrOp result, IrOp target, int param_ct);
void ir_emit_call_indirect(IrOp result, IrOp target, int param_ct); //added for function pointers
void ir_emit_return(IrOp retval);
// TODO: emission functions for arrays/mem access once we get to that in class

/* TODO comment */
void ir_emit_array_read(IrOp result, IrOp arg_arr, IrOp arg_idx);

/* TODO comment */
void ir_emit_array_write(IrOp result_arr, IrOp result_idx, IrOp arg);

/* Like ir_emit_end() except ir continues to be generated */
void ir_emit_seg_fault();

void ir_array_print(FILE* printloc);
#endif

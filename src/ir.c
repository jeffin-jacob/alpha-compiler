#include "../src/ir.h"
#include "stdlib.h"

IrInstr ir_array[4096];
int ir_array_idx = 0;
IrOp OP_NONE = {.kind=IR_OP_NONE};
IrOp OP_PLACEHOLDER = {.kind=IR_OP_PLACEHOLDER_ARRAY_IDX};

//BPList is a LL of indices to backpatch to later
BPList* newBPList(int i) {
    BPList* e = malloc(sizeof(BPList));
    e->idx = i;
    e->next = NULL;
    return e;
}

void addBPElem(BPList* head, int i) {
    BPList* e = malloc(sizeof(BPList));
    e->idx = i;
    e->next = NULL;

    if (head == NULL) {
        head = e;
        return;
    }
    BPList* cur = head;
    while (cur->next != NULL) {
        cur = cur->next;
    }
    cur->next = e;
}

BPList* mergeBPLists(BPList* i1, BPList* i2) {
    if (i1 == NULL) {
        return i2;
    }
    BPList* cur = i1;
    while (cur->next != NULL) {
        cur = cur->next;
    }
    cur->next = i2;
    return i1;
}

void backpatch(BPList* bp_list, int idx) {
    BPList* cur = bp_list;
    while (cur != NULL) {
        IrOp target = ir_op_from_ir_array_entry(idx);
        ir_array[cur->idx].result = target;
        cur=cur->next;
    }
}


IrOp ir_op_from_symEntry(symEntry* e) {
    return (IrOp) {.kind=IR_OP_SYMENTRY, .val.sym_entry=e};
}

IrOp ir_op_from_int(int i) {
    return (IrOp) {.kind=IR_OP_INT, .val.int_val=i};
}

IrOp ir_op_from_bool(bool b) {
    return (IrOp) {.kind=IR_OP_BOOL, .val.bool_val=b};
}

IrOp ir_op_from_char(char c) {
    return (IrOp) {.kind=IR_OP_CHAR, .val.int_val=c};
}

IrOp ir_op_from_ir_array_entry(int i) {
    return (IrOp) {.kind=IR_OP_IR_ARRAY_IDX, .val.ir_array_idx=i};
}

void ir_emit(IrInstr i) {
    ir_array[ir_array_idx] = i;
    ir_array_idx++;
}

void ir_emit_assign(IrOp result, IrOp arg1) {
    IrInstr ir_instr = {.op=IR_ASSIGN, .result=result, .arg1=arg1, .arg2=OP_NONE};
    ir_emit(ir_instr);
}
void ir_emit_func_addr_assign(IrOp result, IrOp fn) {
  //printf("HIT IR_FUNC_ADDR_ASSIGN\n");
    IrInstr ir_instr = {
        .op = IR_FUNC_ADDR_ASSIGN,
        .result = result,
        .arg1 = fn,
        .arg2 = OP_NONE
    };

    ir_emit(ir_instr);
}
void ir_emit_add(IrOp result, IrOp arg1, IrOp arg2) {
    IrInstr ir_instr = {.op=IR_ADD, .result=result, .arg1=arg1, .arg2=arg2};
    ir_emit(ir_instr);
}

void ir_emit_sub(IrOp result, IrOp arg1, IrOp arg2) {
    IrInstr ir_instr = {.op=IR_SUB, .result=result, .arg1=arg1, .arg2=arg2};
    ir_emit(ir_instr);
}

void ir_emit_mul(IrOp result, IrOp arg1, IrOp arg2) {
    IrInstr ir_instr = {.op=IR_MUL, .result=result, .arg1=arg1, .arg2=arg2};
    ir_emit(ir_instr);
}

void ir_emit_div(IrOp result, IrOp arg1, IrOp arg2) {
    IrInstr ir_instr = {.op=IR_DIV, .result=result, .arg1=arg1, .arg2=arg2};
    ir_emit(ir_instr);
}

void ir_emit_rem(IrOp result, IrOp arg1, IrOp arg2) {
    IrInstr ir_instr = {.op=IR_REM, .result=result, .arg1=arg1, .arg2=arg2};
    ir_emit(ir_instr);
}

void ir_emit_neg(IrOp result, IrOp arg1) {
    IrInstr ir_instr = {.op=IR_NEG, .result=result, .arg1=arg1, .arg2=OP_NONE};
    ir_emit(ir_instr);
}

void ir_emit_not(IrOp result, IrOp arg1) {
    IrInstr ir_instr = {.op=IR_NOT, .result=result, .arg1=arg1, .arg2=OP_NONE};
    ir_emit(ir_instr);
}

void ir_emit_and(IrOp result, IrOp arg1, IrOp arg2) {
    IrInstr ir_instr = {.op=IR_AND, .result=result, .arg1=arg1, .arg2=arg2};
    ir_emit(ir_instr);
}

void ir_emit_or(IrOp result, IrOp arg1, IrOp arg2) {
    IrInstr ir_instr = {.op=IR_OR, .result=result, .arg1=arg1, .arg2=arg2};
    ir_emit(ir_instr);
}

void ir_emit_lt(IrOp result, IrOp arg1, IrOp arg2) {
    IrInstr ir_instr = {.op=IR_LT, .result=result, .arg1=arg1, .arg2=arg2};
    ir_emit(ir_instr);
}

void ir_emit_eq(IrOp result, IrOp arg1, IrOp arg2) {
    IrInstr ir_instr = {.op=IR_EQ, .result=result, .arg1=arg1, .arg2=arg2};
    ir_emit(ir_instr);
}

void ir_emit_goto(IrOp target) {
    IrInstr ir_instr = {.op=IR_GOTO, .result=target, .arg1=OP_NONE, .arg2=OP_NONE};
    ir_emit(ir_instr);
}

void ir_emit_if_true(IrOp target, IrOp arg1) {
    IrInstr ir_instr = {.op=IR_IF_TRUE, .result=target, .arg1=arg1, .arg2=OP_NONE};
    ir_emit(ir_instr);
}

void ir_emit_if_false(IrOp target, IrOp arg1) {
    IrInstr ir_instr = {.op=IR_IF_FALSE, .result=target, .arg1=arg1, .arg2=OP_NONE};
    ir_emit(ir_instr);
}

void ir_emit_goto_bp() {
    IrInstr ir_instr = {.op=IR_GOTO, .result=OP_PLACEHOLDER, .arg1=OP_NONE, .arg2=OP_NONE};
    ir_emit(ir_instr);
}

void ir_emit_if_true_bp(IrOp arg1) {
    IrInstr ir_instr = {.op=IR_IF_TRUE, .result=OP_PLACEHOLDER, .arg1=arg1, .arg2=OP_NONE};
    ir_emit(ir_instr);
}

void ir_emit_if_false_bp(IrOp arg1) {
    IrInstr ir_instr = {.op=IR_IF_FALSE, .result=OP_PLACEHOLDER, .arg1=arg1, .arg2=OP_NONE};
    ir_emit(ir_instr);
}

void ir_emit_func_start(IrOp f_name) {
    IrInstr ir_instr = {.op=IR_FUNC_START, .result=f_name, .arg1=OP_NONE, .arg2=OP_NONE};
    ir_emit(ir_instr);
}

void ir_emit_param(IrOp param) {
    IrInstr ir_instr = {.op=IR_PARAM, .result=OP_NONE, .arg1=param, .arg2=OP_NONE};
    ir_emit(ir_instr);
}

void ir_emit_call(IrOp result, IrOp target, int param_ct) {
    IrOp p = {.kind=IR_OP_PARAM_CT, .val.param_ct=param_ct};
    IrInstr ir_instr = {.op=IR_CALL, .result=result, .arg1=target, .arg2=p};
    ir_emit(ir_instr);
}
void ir_emit_call_indirect(IrOp result, IrOp target, int param_ct) {
    IrOp p = {
        .kind = IR_OP_PARAM_CT,
        .val.param_ct = param_ct
    };

    IrInstr ir_instr = {
        .op = IR_CALL_INDIRECT,
        .result = result,
        .arg1 = target,
        .arg2 = p
    };

    ir_emit(ir_instr);
}
void ir_emit_return(IrOp retval) {
    IrInstr ir_instr = {.op=IR_RETURN, .result=OP_NONE, .arg1=retval, .arg2=OP_NONE};
    ir_emit(ir_instr);
}

void ir_emit_end() {
    IrInstr ir_instr = {.op=IR_NOTHING, .result=OP_NONE, .arg1=OP_NONE, .arg2=OP_NONE};
    ir_emit(ir_instr);
}

void ir_emit_array_read(IrOp result, IrOp arg_arr, IrOp arg_idx) {
    IrInstr ir_instr = {.op=IR_ARRAY_ACCESS, .result=result, .arg1=arg_arr, .arg2=arg_idx};
    ir_emit(ir_instr);
}

void ir_emit_array_write(IrOp result_arr, IrOp result_idx, IrOp arg) {
    IrInstr ir_instr = {.op=IR_ARRAY_ASSIGN, .result=result_arr, .arg1=result_idx, .arg2=arg};
    ir_emit(ir_instr);
}

void ir_emit_seg_fault() {
    IrInstr ir_instr = {.op=IR_SEG_FAULT, .result=OP_NONE, .arg1=OP_NONE, .arg2=OP_NONE};
    ir_emit(ir_instr);
}

char* ir_op_as_str(IrOp irop) {
    char* b;
    switch (irop.kind) {
    case IR_OP_SYMENTRY:
        return irop.val.sym_entry->name;
    case IR_OP_INT:
        b = malloc(20*sizeof(char));
        sprintf(b, "%d", irop.val.int_val);
        return b;
    case IR_OP_BOOL:
        return irop.val.bool_val ? "true" : "false";
    case IR_OP_CHAR:
        b = malloc(2*sizeof(char));
        sprintf(b, "%c", irop.val.char_val);
        return b;
    case IR_OP_IR_ARRAY_IDX:
        b = malloc(20*sizeof(char));
        sprintf(b, "[%d]", irop.val.ir_array_idx);
        return b;
    case IR_OP_PLACEHOLDER_ARRAY_IDX:
        return "<backpatch>";
    case IR_OP_PARAM_CT:
        b = malloc(20*sizeof(char));
        sprintf(b, "%d", irop.val.param_ct);
        return b;
    case IR_OP_NONE:
        return "<none>";
    }
    return "you shouldnt be here";
}

void ir_array_print(FILE* printloc) {
    for (int i = 0; i <= 4096; i++) {
        IrInstr instr = ir_array[i];
        char* x = ir_op_as_str(instr.result);
        char* y = ir_op_as_str(instr.arg1);
        char* z = ir_op_as_str(instr.arg2);
        fprintf(printloc, "[%d]: ", i);
        switch (instr.op) {
        case IR_ASSIGN:
            fprintf(printloc, "%s = %s\n", x, y);
            break;
        case IR_ADD:
            fprintf(printloc, "%s = %s + %s\n", x, y, z);
            break;
        case IR_SUB:
            fprintf(printloc, "%s = %s - %s\n", x, y, z);
            break;
        case IR_MUL:
            fprintf(printloc, "%s = %s * %s\n", x, y, z);
            break;
        case IR_DIV:
            fprintf(printloc, "%s = %s / %s\n", x, y, z);
            break;
        case IR_REM:
            fprintf(printloc, "%s = %s %% %s\n", x, y, z);
            break;
        case IR_NEG:
            fprintf(printloc, "%s = -%s\n", x, y);
            break;
        case IR_NOT:
            fprintf(printloc, "%s = !%s\n", x, y);
            break;
        case IR_AND:
            fprintf(printloc, "%s = %s && %s\n", x, y, z);
            break;
        case IR_OR:
            fprintf(printloc, "%s = %s || %s\n", x, y, z);
            break;
        case IR_LT:
            fprintf(printloc, "%s = %s < %s\n", x, y, z);
            break;
        case IR_EQ:
            fprintf(printloc, "%s = %s == %s\n", x, y, z);
            break;
        case IR_GOTO:
            fprintf(printloc, "goto %s\n", x);
            break;
        case IR_IF_TRUE:
            fprintf(printloc, "goto %s if [%s]\n", x, y);
            break;
        case IR_IF_FALSE:
            fprintf(printloc, "goto %s iffalse [%s]\n", x, y);
            break;
        case IR_FUNC_START:
            fprintf(printloc, "function %s:\n", x);
            break;
        case IR_PARAM:
            fprintf(printloc, "param %s\n", y);
            break;
	case IR_FUNC_ADDR_ASSIGN:
	  fprintf(printloc, "%s = &%s\n", x, y);
	  break;

	case IR_CALL_INDIRECT:
	  fprintf(printloc, "%s = call *%s() [%s params]\n", x, y, z);
	  break;	    
        case IR_CALL:
            fprintf(printloc, "%s = %s() [%s params]\n", x, y, z);
            break;
        case IR_RETURN:
            fprintf(printloc, "return %s\n", y);
            break;
        case IR_NOTHING:
            fprintf(printloc, "<end of program>\n");
            return;
        case IR_ARRAY_ACCESS:
            fprintf(printloc, "%s = %s[%s]\n", x, y, z);
            break;
        case IR_ARRAY_ASSIGN:
            fprintf(printloc, "%s[%s] = %s\n", x, y, z);
            break;
        case IR_SEG_FAULT:
            fprintf(printloc, "<segmentation fault>\n");
            break;
        default:
            fprintf(printloc, "not handled yet");
            break;
        }
    }
}

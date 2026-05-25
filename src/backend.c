#include "../src/backend.h"
#include <stdlib.h>
#include <string.h>

#define ARG_REG_COUNT 6
#define ARG_SIZE(N) (8 * N)

AsmInstr asm_array[4096];
int asm_array_idx = 0;
extern bool debugflag;

registerDescriptorEntry registerDescriptor[12];

AsmOp RAX= {.kind = ASM_OP_REG, .val = 12};
AsmOp RAXL = {.kind = ASM_OP_REG, .val = 12,.ss = L};
AsmOp RDX = {.kind = ASM_OP_REG, .val = 13};
AsmOp RDXL = {.kind = ASM_OP_REG, .val = 13, .ss = L};
AsmOp RSP = {.kind = ASM_OP_REG, .val = 14}; 
AsmOp RBP = {.kind = ASM_OP_REG, .val = 15};

AsmOp zeroImm = {.kind = ASM_OP_IMM, .val = 0};
AsmOp oneImm = {.kind = ASM_OP_IMM, .val = 1};

//consts for reg# to str
char* str_regs[16] = {"%rbx", "%rcx", "%rsi", "%rdi", "%r8", "%r9",
                    "%r10", "%r11", "%r12", "%r13", "%r14", "%r15",
                    "%rax", "%rdx", "%rsp", "%rbp"};//these 4 arent in register desc
                                                    //but hardcoded for 
                                                    //div,mod,push,pop,call etc
char *strL_regs[16] = {"%ebx", "%ecx", "%esi", "%edi", "%r8d", "%r9d",
                    "%r10d", "%r11d", "%r12d", "%r13d", "%r14d", "%r15d",
                    "%eax", "%edx", "%esp", "%ebp"};
char *strB_regs[16] = {"%bl", "%cl", "%sil", "%dil", "%r8b", "%r9b",
                    "%r10b", "%r11b", "%r12b", "%r13b", "%r14b", "%r15b",
                    "%al", "%dl", "%sp", "%bp"};
const int arg_regs[] = {3, 2, 13, 1, 4, 5};
int arg_n = 0;

/* get stack size for locals; adapted from dfs() in symbol-table.c */
static int get_stack_size(symbolTable *func) {
    int minOffset = 0;
    symbolTableList *gc;
    symbolTableList *stack = newSymbolTableList(func);
    while (stack != NULL) {
        symbolTable *currNode = stack->elem;
        gc = stack;
        stack = stack->next;
        free(gc);

        /* process */
        symEntry *currEntry = currNode->entry;
        while (currEntry != NULL) {
            int entryOffset = currEntry->offset;
            if (entryOffset < minOffset) {
                minOffset = entryOffset;
            }
            currEntry = currEntry->next;
        }

        symbolTableList *currDescendant = getChildren(currNode);
        while (currDescendant != NULL) {
            symbolTableList *newStack =
                newSymbolTableList(currDescendant->elem);
            newStack->next = stack;
            stack = newStack;
            currDescendant = getRestOfChildren(currDescendant);
        }
    }
            //return offset aligned to 16byte boundary
    return ((abs(minOffset)+15) & ~15);
}

//size suffixes for instructions
char* size_suffixes[3] = {"q", "l", "b"};
void registerContentDump() {
  if (debugflag) {
  printf("registerContentDump started \n");
   for (int i = 0; i < 12; i++) {
        registerDescriptorEntry *curReg = &registerDescriptor[i];
	if(curReg){
        if (curReg->cur == NULL && curReg->next == NULL) {
            continue;
        }

        printf("register: %s\n", str_regs[i]);

        while (curReg != NULL) {
            if (curReg->cur != NULL) {
                const char *name = curReg->cur->name
                    ? curReg->cur->name
                    : "<unnamed>";

                printf("  %s\n", name);
            }

            curReg = curReg->next;
        }
	}
    }
   printf("registerContentDump ended \n");
  }
}

//to save last real instruction in array
int last_ir_instr_idx;
void mark_leaders() { //leaders are first instr, jump targets, instr right after jmp
    ir_array[0].is_leader = true;
    bool next_leader = false;

    int i;
    for (i = 1; ir_array[i].op != IR_NOTHING; i++) {
        if (next_leader) {
            ir_array[i].is_leader = true;
            next_leader = false;
        }

        if (ir_array[i].op == IR_FUNC_START) {
            ir_array[i].is_leader = true;
        }

        if (ir_array[i].op == IR_GOTO || 
            ir_array[i].op == IR_IF_TRUE ||
            ir_array[i].op == IR_IF_FALSE) {
            next_leader = true;
            int jloc = ir_array[i].result.val.ir_array_idx;
            ir_array[jloc].is_leader = true;
        }
        else if (ir_array[i].op == IR_CALL) { //already handled since we have func_start
            next_leader = true;
        }
    }
    //As a side effect, also store the last instruction's index since our loop for liveness and next use needs to iterate backwards thru instruction array
    ir_array[i].is_leader = true;
    last_ir_instr_idx = i-1;
}

void set_lnu() {
    //even tho in class it says to assume temp variables are dead on exit and other
    //local vars/params are live on exit, im just gonna assume everyone is live
    //on exit for simplicity for now
    int i = last_ir_instr_idx;
    while (i > 0) {
        //save end of block to restore later after first pass
        int end_of_block = i;
        //this first loop sets the liveness/next use in symbol table for vars used in this basic block specifically;
        do {
            if (ir_array[i].result.kind == IR_OP_SYMENTRY) { 
                ir_array[i].result.val.sym_entry->alive = true;
                ir_array[i].result.val.sym_entry->next_use = -1;
                ir_array[i].result.val.sym_entry->reg_num = -1;
            }
            if (ir_array[i].arg1.kind == IR_OP_SYMENTRY) { 
                ir_array[i].arg1.val.sym_entry->alive = true;
                ir_array[i].arg1.val.sym_entry->next_use = -1;
                ir_array[i].arg1.val.sym_entry->reg_num = -1;
            }
            if (ir_array[i].arg2.kind == IR_OP_SYMENTRY) { 
                ir_array[i].arg2.val.sym_entry->alive = true;
                ir_array[i].arg2.val.sym_entry->next_use = -1;
                ir_array[i].arg2.val.sym_entry->reg_num = -1;
            }

            i--;
        } while (!ir_array[i].is_leader);

        i = end_of_block;

        do { //MEMOP TODO: when memory operations are
             //added this may need those cases too idk
            switch (ir_array[i].op) {
            //3 operand cases result = arg1 op arg2
            case IR_ADD:
            case IR_SUB:
            case IR_MUL:
            case IR_DIV:
            case IR_REM:
            case IR_AND:
            case IR_OR:
            case IR_LT:
            case IR_EQ:
                if (ir_array[i].arg1.kind == IR_OP_SYMENTRY) {
                    //set local liveness on instruction
                    ir_array[i].arg1LNU.alive = ir_array[i].arg1.val.sym_entry->alive;
                    ir_array[i].arg1LNU.next_use = ir_array[i].arg1.val.sym_entry->next_use;

                    //update symbol table liveness
                    ir_array[i].arg1.val.sym_entry->alive = true;
                    ir_array[i].arg1.val.sym_entry->next_use = i;
                }
                if (ir_array[i].arg2.kind == IR_OP_SYMENTRY) { 
                    //set local liveness on instruction
                    ir_array[i].arg2LNU.alive = ir_array[i].arg2.val.sym_entry->alive;
                    ir_array[i].arg2LNU.next_use = ir_array[i].arg2.val.sym_entry->next_use;

                    //update symbol table liveness
                    ir_array[i].arg2.val.sym_entry->alive = true;
                    ir_array[i].arg2.val.sym_entry->next_use = i;
                }
                if (ir_array[i].result.kind == IR_OP_SYMENTRY) { 
                    //set local liveness on instruction
                    ir_array[i].resultLNU.alive = ir_array[i].result.val.sym_entry->alive;
                    ir_array[i].resultLNU.next_use = ir_array[i].result.val.sym_entry->next_use;

                    //update symbol table liveness
                    ir_array[i].result.val.sym_entry->alive = false;
                    ir_array[i].result.val.sym_entry->next_use = -1;
                }
                break;
            //2 operand cases result = op arg1
            case IR_ASSIGN:
            case IR_NEG:
            case IR_NOT:
                if (ir_array[i].arg1.kind == IR_OP_SYMENTRY) {
                    //set local liveness on instruction
                    ir_array[i].arg1LNU.alive = ir_array[i].arg1.val.sym_entry->alive;
                    ir_array[i].arg1LNU.next_use = ir_array[i].arg1.val.sym_entry->next_use;

                    //update symbol table liveness
                    ir_array[i].arg1.val.sym_entry->alive = true;
                    ir_array[i].arg1.val.sym_entry->next_use = i;
                }
                if (ir_array[i].result.kind == IR_OP_SYMENTRY) { 
                    //set local liveness on instruction
                    ir_array[i].resultLNU.alive = ir_array[i].result.val.sym_entry->alive;
                    ir_array[i].resultLNU.next_use = ir_array[i].result.val.sym_entry->next_use;

                    //update symbol table liveness
                    ir_array[i].result.val.sym_entry->alive = false;
                    ir_array[i].result.val.sym_entry->next_use = -1;
                }
                break;
            default:
                break;
            }
            i--;
        } while (!ir_array[i].is_leader);
    }
}

// updates register descriptor and symEntry reg_num when assigning a variable to a register
void assign_to_reg(int reg, symEntry *entry, bool overwrite) {
    //if should overwrite register rather than add new elem to linkedlist
    if (overwrite) {
        registerDescriptor[reg].cur = entry;
        registerDescriptor[reg].next = NULL;

        if (entry != NULL) {
	  entry->reg_num = reg;
        }
        return;
    }

    registerDescriptorEntry* r = &registerDescriptor[reg];
    if (r->cur == NULL) {
        r->cur = entry;
        r->next = NULL;
	entry->reg_num = reg;
        return;
    }

    while (r->next != NULL) {
        r = r->next;
    } //get to tail of linkedlist
    registerDescriptorEntry* newRDE = malloc(sizeof(registerDescriptorEntry));
    newRDE->cur = entry;
    newRDE->next = NULL;
    r->next = newRDE;

    // update entry's reg_num
    if (entry != NULL) {
        entry->reg_num = reg;
    }
}

void restore_lnu(IrInstr instr) {
    if (instr.arg1.kind == IR_OP_SYMENTRY) {
        instr.arg1.val.sym_entry->alive = instr.arg1LNU.alive;
        instr.arg1.val.sym_entry->next_use = instr.arg1LNU.next_use;
    }
    if (instr.arg2.kind == IR_OP_SYMENTRY) {
        instr.arg2.val.sym_entry->alive = instr.arg2LNU.alive;
        instr.arg2.val.sym_entry->next_use = instr.arg2LNU.next_use;
    }
    if (instr.result.kind == IR_OP_SYMENTRY) {
        instr.result.val.sym_entry->alive = instr.resultLNU.alive;
        instr.result.val.sym_entry->next_use = instr.resultLNU.next_use;
    }
}

AsmOp RBP_offset(int offset) {
    AsmOp r = {.kind = ASM_OP_OFFSET, .val = 15, .offset = offset}; // 15 = rbp
    return r;
}

// generates a store instruction if the variable in this register
// has been modified and needs to be written back to memory
void spill(int reg) {
    registerDescriptorEntry* r = &registerDescriptor[reg];

    if (r->cur == NULL) {
        return;
    }

    while (r != NULL) {
        symEntry* v = r->cur;

        if (v != NULL) {
            if (v->not_in_memory) {
	      AsmOp src = {.kind = ASM_OP_REG, .val = reg, .ss =  get_size_ss(v)};
	      AsmOp dst = RBP_offset(v->offset);
		char sizeSrc;
		switch(src.ss){
		case Q:
		  sizeSrc = 'Q';
		  break;
		case L:
		  sizeSrc = 'L';
		  break;
		case B:
		  sizeSrc = 'B';
		  break;
		}
		char sizeSym;
		switch(get_size_ss(v)){
                case Q:
                  sizeSym = 'Q';
		  break;
                case L:
                  sizeSym = 'L';
		  break;
                case B:
                  sizeSym = 'B';
		  break;
                }
          if (debugflag) {
		    printf("storing %s offset=%d reg=%s sizeSrc=%c symEntrySize=%c \n",v->name, v->offset,str_regs[reg], sizeSrc, sizeSym);
          }

                asm_emit_mov_ss(dst, src, get_size_ss(v));
                v->not_in_memory = false;
            }

           if (v->reg_num == reg) {
                v->reg_num = -1;
            }
	     r->cur = NULL; 
	}

        r = r->next;
    }
}

// function to spill all registers into memory and assume registers empty at the start of a basic block
void spill_all_regs() {
  registerContentDump();
    for (int i = 0; i < 12; i++) { //go through registers
        spill(i);
        registerDescriptor[i].cur = NULL;
        registerDescriptor[i].next = NULL;
    }
}
reg3 get_reg3Clean(IrInstr instr) {
    reg3 r = {.result = 0, .arg1 = 0, .arg2 = 0};

    symEntry *baseEntry  = instr.result.kind == IR_OP_SYMENTRY ? instr.result.val.sym_entry : NULL;
    symEntry *idxEntry   = instr.arg1.kind   == IR_OP_SYMENTRY ? instr.arg1.val.sym_entry   : NULL;
    symEntry *valueEntry = instr.arg2.kind   == IR_OP_SYMENTRY ? instr.arg2.val.sym_entry   : NULL;

    if (baseEntry == NULL || idxEntry == NULL || valueEntry == NULL) {
        if (debugflag) {
          printf("one of these is null in get_reg3Clean\n");
        }
        return r;
    }

    // ---------- BASE: always put clean copy in r.result ----------
    int baseOldReg = baseEntry->reg_num;
    int baseReg = -1;

    for (int i = 0; i < 12; i++) {
        if (i == idxEntry->reg_num || i == valueEntry->reg_num) continue;
        if (registerDescriptor[i].cur == NULL) {
            baseReg = i;
            break;
        }
    }

    if (baseReg == -1) {
        int best_reg = 0;
        int best_score = 999;

        for (int i = 0; i < 12; i++) {
            if (i == idxEntry->reg_num || i == valueEntry->reg_num) continue;

            int score = 0;
            registerDescriptorEntry *rdEntry = &registerDescriptor[i];

            while (rdEntry != NULL) {
                if (rdEntry->cur == NULL) {
                } else if (!rdEntry->cur->not_in_memory) {
                } else if (!rdEntry->cur->alive) {
                } else {
                    score++;
                }
                rdEntry = rdEntry->next;
            }

            if (score < best_score) {
                best_score = score;
                best_reg = i;
            }
        }

        baseReg = best_reg;
        spill(baseReg);
    }

    r.result = baseReg;

    if (baseOldReg != -1) {
        AsmOp src = {.kind = ASM_OP_REG, .val = baseOldReg, .ss = Q};
        AsmOp dst = {.kind = ASM_OP_REG, .val = r.result,   .ss = Q};

        if (baseOldReg != r.result) {
            asm_emit_mov_ss(dst, src, Q);
        }
    } else {
        AsmOp src = RBP_offset(baseEntry->offset);
        AsmOp dst = {.kind = ASM_OP_REG, .val = r.result, .ss = Q};
        asm_emit_mov_ss(dst, src, Q);
    }

    //assign_to_reg(r.result, baseEntry, true);

    // ---------- INDEX: reuse if valid, otherwise load ----------
    if (idxEntry->reg_num != -1 && idxEntry->reg_num != r.result) {
        r.arg1 = idxEntry->reg_num;
    } else {
        int idxReg = -1;

        for (int i = 0; i < 12; i++) {
            if (i == r.result || i == valueEntry->reg_num) continue;
            if (registerDescriptor[i].cur == NULL) {
                idxReg = i;
                break;
            }
        }

        if (idxReg == -1) {
            int best_reg = 0;
            int best_score = 999;

            for (int i = 0; i < 12; i++) {
                if (i == r.result || i == valueEntry->reg_num) continue;

                int score = 0;
                registerDescriptorEntry *rdEntry = &registerDescriptor[i];

                while (rdEntry != NULL) {
                    if (rdEntry->cur == NULL) {
                    } else if (!rdEntry->cur->not_in_memory) {
                    } else if (!rdEntry->cur->alive) {
                    } else {
                        score++;
                    }
                    rdEntry = rdEntry->next;
                }

                if (score < best_score) {
                    best_score = score;
                    best_reg = i;
                }
            }

            idxReg = best_reg;
            spill(idxReg);
        }

        r.arg1 = idxReg;

        AsmOp src = RBP_offset(idxEntry->offset);
        AsmOp dst = {.kind = ASM_OP_REG, .val = r.arg1, .ss = get_size_ss(idxEntry)};
        asm_emit_mov_ss(dst, src, get_size_ss(idxEntry));

        assign_to_reg(r.arg1, idxEntry, true);
    }

    // ---------- VALUE: reuse if valid, otherwise load ----------
    if (valueEntry->reg_num != -1 &&
        valueEntry->reg_num != r.result &&
        valueEntry->reg_num != r.arg1) {
        r.arg2 = valueEntry->reg_num;
    } else {
        int valReg = -1;

        for (int i = 0; i < 12; i++) {
            if (i == r.result || i == r.arg1) continue;
            if (registerDescriptor[i].cur == NULL) {
                valReg = i;
                break;
            }
        }

        if (valReg == -1) {
            int best_reg = 0;
            int best_score = 999;

            for (int i = 0; i < 12; i++) {
                if (i == r.result || i == r.arg1) continue;

                int score = 0;
                registerDescriptorEntry *rdEntry = &registerDescriptor[i];

                while (rdEntry != NULL) {
                    if (rdEntry->cur == NULL) {
                    } else if (!rdEntry->cur->not_in_memory) {
                    } else if (!rdEntry->cur->alive) {
                    } else {
                        score++;
                    }
                    rdEntry = rdEntry->next;
                }

                if (score < best_score) {
                    best_score = score;
                    best_reg = i;
                }
            }

            valReg = best_reg;
            spill(valReg);
        }

        r.arg2 = valReg;

        AsmOp src = RBP_offset(valueEntry->offset);
        AsmOp dst = {.kind = ASM_OP_REG, .val = r.arg2, .ss = get_size_ss(valueEntry)};
        asm_emit_mov_ss(dst, src, get_size_ss(valueEntry));

        assign_to_reg(r.arg2, valueEntry, true);
    }

    restore_lnu(instr);
    return r;
}
// reg3 function handles any loads that need to be done beforehand of arg1 and arg2
// as well as spills
// 1) load arg1's value into result reg
// 2) load arg2's value into arg2 reg
// after getting registers with reg3 the procedure is as follows:
// 1) emit a copy instruction of result to arg1
// 2) emit the binary operation with result and arg2
reg3 get_reg3(IrInstr instr) {
    reg3 r = {.result = 0, .arg1 = 0, .arg2 = 0};

    symEntry *arg1Entry = instr.arg1.kind == IR_OP_SYMENTRY ? instr.arg1.val.sym_entry : NULL;
    symEntry *arg2Entry = instr.arg2.kind == IR_OP_SYMENTRY ? instr.arg2.val.sym_entry : NULL;
    symEntry *resultEntry = instr.result.kind == IR_OP_SYMENTRY ? instr.result.val.sym_entry : NULL;

    if (arg1Entry == NULL || arg2Entry == NULL || resultEntry == NULL) {if (debugflag) {printf("one of these is null, returning 0 \n");} return r; }

    if (arg1Entry->reg_num != -1) { //if already in register no load needed
        r.result = arg1Entry->reg_num;
    } else {
        int free_reg = -1;
        for (int i = 0; i < 12; i++) { //if empty register, take it
            if (registerDescriptor[i].cur == NULL) {free_reg = i; break; }
        }
        if (free_reg != -1) {
            r.result = free_reg;
        } else {
            int best_reg = 0;
            int best_score = 999;
            for (int i = 0; i < 12; i++) { //go through registers trying to find 'cheapest'
                int score = 0;
                registerDescriptorEntry* rdEntry = &registerDescriptor[i];
                while (rdEntry != NULL) { //for every variable that may be in that register
                    if (!rdEntry->cur->not_in_memory) { //if value already in memory we're fine
                    } else if (!rdEntry->cur->alive) { //if value in register is dead we're fine
                    } else if (rdEntry->cur == arg2Entry) { //dont override arg2 entry no matter what
                        score += 1000;
                    } else {
                        score++;
                    }
                    rdEntry = rdEntry->next;
                }
                if (score < best_score) {
                    best_reg = i;
                    best_score = score;
                }
            }
            //once you have cheapest register, perform necessary spills
	    spill(best_reg);
            r.result = best_reg;
        }
        //now that we have register, perform the load
        AsmOp src = RBP_offset(arg1Entry->offset);
        AsmOp dst = {.kind = ASM_OP_REG, .val = r.result,.ss = get_size_ss(arg1Entry)};
        asm_emit_mov_ss(dst, src, get_size_ss(arg1Entry));
    }
    // update descriptors

    if (arg2Entry->reg_num != -1) { //if already in register no load needed
        r.arg2 = arg2Entry->reg_num;
    } else {
        int free_reg = -1;
        for (int i = 0; i < 12; i++) { //if empty register, take it
            if (i == r.result) {continue; }
            if (registerDescriptor[i].cur == NULL) {free_reg = i; break; }
        }
        if (free_reg != -1) {
            r.arg2 = free_reg;
        } else {
            int best_reg = 0;
            int best_score = 999;
            for (int i = 0; i < 12; i++) { //go through registers trying to find 'cheapest'
                if (i == r.result) {continue; }
                int score = 0;
                registerDescriptorEntry* rdEntry = &registerDescriptor[i];
                while (rdEntry != NULL) { //for every variable that may be in that register
                    if (!rdEntry->cur->not_in_memory) { //if value already in memory we're fine
                    } else if (!rdEntry->cur->alive) { //if value in register is dead we're fine
                    } else if (rdEntry->cur == arg1Entry || rdEntry->cur == resultEntry) { 
                        //dont override arg1 or result entry no matter what
                        score += 1000;
                    } else {
                        score++;
                    }
                    rdEntry = rdEntry->next;
                }
                if (score < best_score) {
                    best_reg = i;
                    best_score = score;
                }
            }
            //once you have cheapest register, perform necessary spills
            spill(best_reg);
	    r.arg2 = best_reg;
        }
        //now that we have register, perform the load
        AsmOp src = RBP_offset(arg2Entry->offset);
        AsmOp dst = {.kind = ASM_OP_REG, .val = r.arg2,.ss = get_size_ss(arg2Entry)};
        asm_emit_mov_ss(dst, src, get_size_ss(arg2Entry));
    }
    // update descriptors
    assign_to_reg(r.arg2, arg2Entry, true);

    if (resultEntry->reg_num != -1) { //if already in register no load needed
        r.arg1 = resultEntry->reg_num;
    } else {
        int free_reg = -1;
        for (int i = 0; i < 12; i++) { //if empty register, take it
            if (i == r.result) {continue; }
            if (registerDescriptor[i].cur == NULL) {free_reg = i; break; }
        }
        if (free_reg != -1) {
            r.arg1 = free_reg;
        } else {
            int best_reg = 0;
            int best_score = 999;
            for (int i = 0; i < 12; i++) { //go through registers trying to find 'cheapest'
                if (i == r.result) {continue; }
                int score = 0;
                registerDescriptorEntry* rdEntry = &registerDescriptor[i];
                while (rdEntry != NULL) { //for every variable that may be in that register
                    if (!rdEntry->cur->not_in_memory) { //if value already in memory we're fine
                    } else if (!rdEntry->cur->alive) { //if value in register is dead we're fine
                    } else if (rdEntry->cur == arg1Entry || rdEntry->cur == arg2Entry) { 
                        //dont override arg1 or arg2 entry no matter what
                        score += 1000;
                    } else {
                        score++;
                    }
                    rdEntry = rdEntry->next;
                }
                if (score < best_score) {
                    best_reg = i;
                    best_score = score;
                }
            }
            //once you have cheapest register, perform necessary spills
	    spill(best_reg);
            r.arg1 = best_reg;
        }
        //we don't need load for this'un since we're going to do a copy into this register right before binop
    }
    // update descriptors
    assign_to_reg(r.result, resultEntry, true);
    assign_to_reg(r.arg1, arg1Entry, true);

    // restore liveness/next_use info of this instr into symbol table
    restore_lnu(instr);

    return r;
}

// reg2 function also handles any loads that need to be done beforehand of arg1
// as well as spills
// 1) load arg1's value into result reg
// after getting registers with reg2 the procedure is as follows:
// 1) emit a copy instruction of result to arg1
// 2) emit the unary operation with just result
reg2 get_reg2(IrInstr instr) {
    reg2 r = {.result = 0, .arg1 = 0};

    symEntry *arg1Entry = instr.arg1.kind == IR_OP_SYMENTRY ? instr.arg1.val.sym_entry : NULL;
    symEntry *resultEntry = instr.result.kind == IR_OP_SYMENTRY ? instr.result.val.sym_entry : NULL;

    if (arg1Entry == NULL || resultEntry == NULL) {if (debugflag) {printf("one of these is null, returning 0 \n");}  return r;}

    if (arg1Entry->reg_num != -1) { //if already in register no load needed
        r.result = arg1Entry->reg_num;
    } else {
        int free_reg = -1;
        for (int i = 0; i < 12; i++) { //if empty register, take it
	  if (registerDescriptor[i].cur == NULL) {free_reg = i; if (debugflag) {printf("found free register: %d \n", i);} break; }
        }
        if (free_reg != -1) {
            r.result = free_reg;
        } else {
            int best_reg = 0;
            int best_score = 999;
            for (int i = 0; i < 12; i++) { //go through registers trying to find 'cheapest'
                int score = 0;
                registerDescriptorEntry* rdEntry = &registerDescriptor[i];
                while (rdEntry != NULL) { //for every variable that may be in that register
                    if (!rdEntry->cur->not_in_memory) { //if value already in memory we're fine
                    } else if (!rdEntry->cur->alive) { //if value in register is dead we're fine
                    } else {
                        score++;
                    }
                    rdEntry = rdEntry->next;
                }
                if (score < best_score) {
                    best_reg = i;
                    best_score = score;
                }
            }
            //once you have cheapest register, perform necessary spills
	    spill(best_reg);
            r.result = best_reg;
        }
        //now that we have register, perform the load
        AsmOp src = RBP_offset(arg1Entry->offset);
        AsmOp dst = {.kind = ASM_OP_REG, .val = r.result,.ss = get_size_ss(arg1Entry)};
        asm_emit_mov_ss(dst, src, get_size_ss(resultEntry));
    }

    if (resultEntry->reg_num != -1) { //if already in register no load needed
        r.arg1 = resultEntry->reg_num;
    } else {
        int free_reg = -1;
        for (int i = 0; i < 12; i++) { //if empty register, take it
            if (i == r.result) {continue; }
            if (registerDescriptor[i].cur == NULL) {free_reg = i; if (debugflag) {printf("found free register: %d \n", i);} break; }
        }
        if (free_reg != -1) {
            r.arg1 = free_reg;
        } else {
            int best_reg = 0;
            int best_score = 999;
            for (int i = 0; i < 12; i++) { //go through registers trying to find 'cheapest'
                if (i == r.result) {continue; }
                int score = 0;
                registerDescriptorEntry* rdEntry = &registerDescriptor[i];
                while (rdEntry != NULL) { //for every variable that may be in that register
                    if (!rdEntry->cur->not_in_memory) { //if value already in memory we're fine
                    } else if (!rdEntry->cur->alive) { //if value in register is dead we're fine
                    } else if (rdEntry->cur == arg1Entry) { 
                        //dont override arg1 entry no matter what
                        score += 1000;
                    } else {
                        score++;
                    }
                    rdEntry = rdEntry->next;
                }
                if (score < best_score) {
                    best_reg = i;
                    best_score = score;
                }
            }
            //once you have cheapest register, perform necessary spills
	    spill(best_reg);
            r.arg1 = best_reg;
        }
        //we don't need load for this'un since we're going to do a copy into this register right before binop
    }
    // update descriptors
    assign_to_reg(r.result, resultEntry, true);
    assign_to_reg(r.arg1, arg1Entry, true);

    // restore liveness/next_use info of this instr into symbol table
    restore_lnu(instr);
    return r;
}


// this is only really necessary for assign ops where a constant is being assigned to a variable
int get_reg1(IrInstr instr) {
    int r = 0;
    symEntry *resultEntry = instr.result.kind == IR_OP_SYMENTRY ? instr.result.val.sym_entry : NULL;

    if (resultEntry == NULL) { return r; }

    if (resultEntry->reg_num != -1) { //if already in register no load needed
        r = resultEntry->reg_num;
    } else {
        int free_reg = -1;
        for (int i = 0; i < 12; i++) { //if empty register, take it
            if (registerDescriptor[i].cur == NULL) {free_reg = i; break; }
        }
        if (free_reg != -1) {
            r = free_reg;
        } else {
            int best_reg = 0;
            int best_score = 999;
            for (int i = 0; i < 12; i++) { //go through registers trying to find 'cheapest'
                int score = 0;
                registerDescriptorEntry* rdEntry = &registerDescriptor[i];
                while (rdEntry != NULL) { //for every variable that may be in that register
                    if (!rdEntry->cur->not_in_memory) { //if value already in memory we're fine
                    } else if (!rdEntry->cur->alive) { //if value in register is dead we're fine
                    } else {
                        score++;
                    }
                    rdEntry = rdEntry->next;
                }
                if (score < best_score) {
                    best_reg = i;
                    best_score = score;
                }
            }
            //once you have cheapest register, perform necessary spills
	    //printf("spilling for result in get_reg1 \n");
	    spill(best_reg);
            r = best_reg;
        }
	
    }
    // update descriptors
    assign_to_reg(r, resultEntry, true);

    // restore liveness/next_use info of this instr into symbol table
    restore_lnu(instr);
    return r;
}

// same as get_reg1 but for arg1 instead of result
int get_reg1_arg1(IrInstr instr) {
    int r = 0;
    symEntry *arg1Entry = instr.arg1.kind == IR_OP_SYMENTRY ? instr.arg1.val.sym_entry : NULL;

    if (arg1Entry == NULL) { return r; }

    if (arg1Entry->reg_num != -1) { //if already in register no load needed
        r = arg1Entry->reg_num;
    } else {
        int free_reg = -1;
        for (int i = 0; i < 12; i++) { //if empty register, take it
            if (registerDescriptor[i].cur == NULL) {free_reg = i; break; }
        }
        if (free_reg != -1) {
            r = free_reg;
        } else {
            int best_reg = 0;
            int best_score = 999;
            for (int i = 0; i < 12; i++) { //go through registers trying to find 'cheapest'
                int score = 0;
                registerDescriptorEntry* rdEntry = &registerDescriptor[i];
                while (rdEntry != NULL) { //for every variable that may be in that register
                    if (!rdEntry->cur->not_in_memory) { //if value already in memory we're fine
                    } else if (!rdEntry->cur->alive) { //if value in register is dead we're fine
                    } else {
                        score++;
                    }
                    rdEntry = rdEntry->next;
                }
                if (score < best_score) {
                    best_reg = i;
                    best_score = score;
                }
            }
            //once you have cheapest register, perform necessary spills
	    spill(best_reg);
            r = best_reg;
        }
        //now that we have register, perform the load
        AsmOp src = RBP_offset(arg1Entry->offset);
        AsmOp dst = {.kind = ASM_OP_REG, .val = r,.ss = get_size_ss(arg1Entry)};
        asm_emit_mov_ss(dst, src, get_size_ss(arg1Entry));
    }
    // update descriptors
    assign_to_reg(r, arg1Entry, true);

    // restore liveness/next_use info of this instr into symbol table
    restore_lnu(instr);
    return r;
}



int codegen(FILE* cg_file) {
    // mark leaders separates into basic blocks
    mark_leaders();
    // set liveness and next use for variables at each instruction to be used in get_reg
    set_lnu();

    int i;
    bool shouldSpill = true;
    //convert IR into assembly loop
    for (i = 0; ir_array[i].op != IR_NOTHING; i++) {

        //Adding this here so regs are always spilled, ugly but just to see if code works
        IrInstr cur = ir_array[i];
	bool hasntSpilled = true;
	if(debugflag && 81 < i && i < 90){
	  printf("displaying registers at ir op %d \n", i);
	  registerContentDump();
	}
        if (ir_array[i].is_leader) {
            AsmOp l = {.kind = ASM_OP_LABEL, .val = i};
	    asm_emit_label(l);
        }

        AsmOp dst;
        AsmOp src1;
        AsmOp src2;

        reg3 regs3;
        reg2 regs2;
        
        switch(cur.op) { //long ass switch statement
        //3 op instructions besides division and remainders
        //since those have to be handled weirdly
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_AND:
        case IR_OR: {
            regs3 = get_reg3(cur);
            dst.kind = ASM_OP_REG;
            dst.val = regs3.result;
            src1.kind = ASM_OP_REG;
            src1.val = regs3.arg1;
            src2.kind = ASM_OP_REG;
            src2.val = regs3.arg2;
	    int ss = get_size_ss(cur.result.val.sym_entry);
	    dst.ss = ss;
	    src1.ss = ss;
	    src2.ss = ss;
	    asm_emit_mov_ss(src1, dst, ss);
            //asm_emit_mov_ss(src1, dst,L); //emit copy FROM result reg TO src1 reg (result reg is prev src1)
            if (cur.op == IR_ADD) asm_emit_add(dst, src2);
            else if (cur.op == IR_SUB) asm_emit_sub(dst, src2);
            else if (cur.op == IR_MUL) asm_emit_imul(dst, src2);
            else if (cur.op == IR_AND) asm_emit_and(dst, src2);
            else if (cur.op == IR_OR)  asm_emit_or(dst, src2);
            cur.result.val.sym_entry->not_in_memory = true;
            break;
        }
        case IR_NEG:
        case IR_NOT: {
            regs2 = get_reg2(cur);
            dst.kind = ASM_OP_REG;
            dst.val = regs2.result;
            src1.kind = ASM_OP_REG;
            src1.val = regs2.arg1;
	    int ss = get_size_ss(cur.result.val.sym_entry);
	    dst.ss = ss;
            src1.ss = ss;
            asm_emit_mov_ss(src1, dst,ss); //emit copy FROM result reg TO src1 reg (result reg is prev src1)
            if (cur.op == IR_NEG) asm_emit_neg(dst);
            else asm_emit_not(dst);
            cur.result.val.sym_entry->not_in_memory = true;
            break;
        }
        case IR_DIV:
        case IR_REM:
            regs3 = get_reg3(cur);
            dst.kind = ASM_OP_REG;
            dst.val = regs3.result;
            src1.kind = ASM_OP_REG;
            src1.val = regs3.arg1;
            src2.kind = ASM_OP_REG;
            src2.val = regs3.arg2;
	    dst.ss = L;
	    src1.ss = L;
	    src2.ss = L;
            asm_emit_mov_ss(src1, dst,L); //copy src1 into its new register
            asm_emit_mov_ss(RAXL, dst,L); //copy src1 into RAX
            asm_emit_cltd(); //sign extend RAX to RDX
            asm_emit_idiv(src2); //do division with src2
            if (cur.op == IR_DIV) asm_emit_mov_ss(dst, RAXL,L); //copy RAX back into dst if should div
            if (cur.op == IR_REM) asm_emit_mov_ss(dst, RDXL,L); //copy RDX back into dst if should mod
            cur.result.val.sym_entry->not_in_memory = true;
            break;

	case IR_LT:
	  regs3 = get_reg3(cur);

	  dst.kind = ASM_OP_REG;
	  dst.val = regs3.result;
	  dst.ss = L;

	  src1.kind = ASM_OP_REG;
	  src1.val = regs3.arg1;
	  src1.ss = L;

	  src2.kind = ASM_OP_REG;
	  src2.val = regs3.arg2;
	  src2.ss = L;

	  AsmOp oneReg;
	  oneReg.kind = ASM_OP_REG;
	  oneReg.val = 12;
	  oneReg.ss = L;

	  zeroImm.ss = L;
	  oneImm.ss = L;

	  // Preserve old arg1 into src1.
	  asm_emit_mov_ss(src1, dst, L);
	  assign_to_reg(src1.val, cur.arg1.val.sym_entry, true);
	  cur.arg1.val.sym_entry->reg_num = src1.val;

	  // Compare preserved arg1 against arg2.
	  asm_emit_cmp_ss(src1, src2, L);

	  // Compute boolean into dst.
	  asm_emit_mov_ss(dst, zeroImm, L);
	  asm_emit_mov_ss(oneReg, oneImm, L);
	  asm_emit_cmovl(dst, oneReg);

	  // dst now contains only the boolean result.
	  assign_to_reg(dst.val, cur.result.val.sym_entry, true);
	  cur.result.val.sym_entry->reg_num = dst.val;
	  cur.result.val.sym_entry->not_in_memory = true;

	  break;

        case IR_EQ:
            regs3 = get_reg3(cur);
            dst.kind = ASM_OP_REG;
            dst.val = regs3.result;
            src1.kind = ASM_OP_REG;
            src1.val = regs3.arg1;
            src2.kind = ASM_OP_REG;
            src2.val = regs3.arg2;
	    int ss = get_size_ss(cur.arg1.val.sym_entry);
	    src1.ss = ss;
	    src2.ss = ss;
	    dst.ss = ss;
            asm_emit_mov_ss(src1, dst,ss); //emit copy FROM result reg TO src1 reg (result reg is prev src1)
            asm_emit_cmp_ss(dst, src2,ss); //dst-src2 -> set flags
            asm_emit_mov_ss(dst, zeroImm,L); //set dst to 0 by default
            asm_emit_mov_ss(RAXL, oneImm,L);  //move 1 into RAX
            asm_emit_cmove(dst, RAXL);   //conditionally copy RAX (1 imm) into dst if dst-src=0
            cur.result.val.sym_entry->not_in_memory = true;
            break;

        //assign is handled differently w/ register sharing
        case IR_ASSIGN:
	  //printf("IR_ASSIGN cur.result: %s \n", cur.result.val.sym_entry -> name);
            if (cur.arg1.kind == IR_OP_SYMENTRY) {
                //get register of argument
	        //printf("IR_ASSIGN cur.arg1: %s \n", cur.arg1.val.sym_entry -> name);
                int r = get_reg1_arg1(cur);
		//register sharing, nothing emitted, instead register updated to also point to result
		assign_to_reg(r, cur.result.val.sym_entry, false);

	    } else {
                //assigning an immediate value into a register
                dst.kind = ASM_OP_REG;
                dst.val = get_reg1(cur);
                src1.kind = ASM_OP_IMM;
		int curSS = Q;
		switch (cur.arg1.kind) {
                case IR_OP_INT:
                    src1.val = cur.arg1.val.int_val;
		    curSS = L;
                    break;
                case IR_OP_BOOL:
                    src1.val = cur.arg1.val.bool_val;
		    curSS = B;
                    break;
                case IR_OP_CHAR:
                    src1.val = cur.arg1.val.char_val;
		    curSS = B;
                    break;

                default:
                    src1.kind = ASM_OP_NONE;
                    //this should never be reached
                }
		//     asm_emit_mov(dst, src1);
		dst.ss = curSS;
		//src1.ss = curSS;
		asm_emit_mov_ss(dst, src1, curSS);
            }
            cur.result.val.sym_entry->not_in_memory = true;
            break;
	case IR_FUNC_ADDR_ASSIGN: {
	  // result = function pointer variable
	  // arg1 = actual function symbol
	  //printf("HIT IR_FUNC_ADDR_ASSIGN in backend\n");
	  dst.kind = ASM_OP_REG;
	  dst.val = get_reg1(cur);   // register for result
	  dst.ss = Q;

	  asm_emit_leaq_func(dst, cur.arg1.val.sym_entry);
	  assign_to_reg(dst.val, cur.result.val.sym_entry, false);

	  cur.result.val.sym_entry->not_in_memory = true;
	  break;
	}
        case IR_IF_TRUE: {
	  //int r = get_reg1_arg1(cur);

	  /*
	    Force condition to memory before branch.
	    This prevents compare using the wrong register after block-boundary cleanup.
	  */
	  hasntSpilled = false;
	  spill_all_regs();

	  AsmOp compareVal = RBP_offset(cur.arg1.val.sym_entry->offset);
	  compareVal.ss = B;

	  AsmOp oneImm = {.kind = ASM_OP_IMM, .val = 1, .ss = B};
	  asm_emit_cmp_ss(compareVal, oneImm, B);

	  AsmOp jtarget = {.kind = ASM_OP_LABEL, .val = cur.result.val.ir_array_idx};
	  asm_emit_je(jtarget);
	  break;
	}

	case IR_IF_FALSE:
	case IR_GOTO: {
	  hasntSpilled = false;
	  spill_all_regs();

	  AsmOp target = {.kind = ASM_OP_LABEL, .val = cur.result.val.ir_array_idx};
	  asm_emit_jmp(target);
	  break;
	}
        case IR_FUNC_START:

            /* emit function label */
            symEntry *func;
            if (cur.result.kind == IR_OP_SYMENTRY) {
                func = cur.result.val.sym_entry;
            }
            dst.kind = ASM_OP_FUNC_LABEL;
            dst.func_entry = func;
            asm_emit_label(dst);

            /* push base pointer onto stack */
            src1 = RBP;
            asm_emit_push(src1);

            /* move contents of stack pointer into base pointer */
            dst = RBP;
            src1 = RSP;
            asm_emit_mov(dst, src1);

            /* calculate parameter count and stack size, for use later */
            int param_count = 0;
            int stack_size = 0;
            symbolTable *scope = get_function_table(func);
            if (scope != NULL) {
                symEntry *curr_node = scope->entry;
                while (curr_node != NULL) {
                    if (curr_node->paramOf == func) {
                        param_count++;
                    }
                    curr_node = curr_node->next;
                }
                stack_size = get_stack_size(scope);
            }

            /* spill args into positive offsets relative to bp */
            dst.kind = ASM_OP_OFFSET;
            dst.val = RBP.val;
            src1.kind = ASM_OP_REG;
            for (int i = 0; i < param_count; i++) {
                if (i < ARG_REG_COUNT) {
                    dst.offset = ARG_SIZE(i) + 16; // account for return address
                    src1.val = arg_regs[i];
                    asm_emit_mov(dst, src1);
                } else {
                    asm_emit_nop();
                }
            }

            /* allocate stack size */
            dst = RSP;
            src1.kind = ASM_OP_IMM;
            src1.val = stack_size;
            asm_emit_sub_ss(dst, src1, Q);
            
            break;
        
        case IR_PARAM:

            /* spill regs */
	   spill_all_regs();

            /* populate registers with arguments */
            if (arg_n < ARG_REG_COUNT) {
                dst.kind = ASM_OP_REG;
                dst.ss = Q;
                dst.val = arg_regs[arg_n];
                src1.kind = ASM_OP_REG;
                src1.val = get_reg1_arg1(cur);
                src1.ss = Q;
                asm_emit_mov(dst, src1);
                arg_n++;
            } else {
                asm_emit_nop(); // ignore argument 7 and above
            }

            break;

        case IR_CALL:
            
            /* subtract necessary space from sp for args */
            dst = RSP;
            src1.kind = ASM_OP_IMM;
            int arg_size = ARG_SIZE(arg_n);
            if ((arg_size % 16) == 0) {  // check alignment
                src1.val = arg_size;
            } else {
                src1.val = arg_size + 8; // add padding to preserve alignment
            }
            asm_emit_sub_ss(dst, src1, Q);

            /* function call */
            dst.kind = ASM_OP_FUNC_LABEL;
            if (cur.arg1.kind == IR_OP_SYMENTRY) {
                dst.func_entry = cur.arg1.val.sym_entry;
            }

            asm_emit_call(dst);

            asm_emit_add_ss(RSP, src1, Q);
            /* process return value */
            dst.kind = ASM_OP_REG;
            dst.val = get_reg1(cur);
            src1 = RAX;
            asm_emit_mov(dst, src1);

            cur.result.val.sym_entry->not_in_memory = true;

            arg_n = 0;
            break;
	case IR_CALL_INDIRECT: {
	  /* subtract necessary space from sp for args */
	  dst = RSP;
	  src1.kind = ASM_OP_IMM;

	  int arg_size = ARG_SIZE(arg_n);
	  if ((arg_size % 16) == 0) {
	    src1.val = arg_size;
	  } else {
	    src1.val = arg_size + 8;
	  }

	  asm_emit_sub_ss(dst, src1, Q);

	  /*
	   * Load function pointer into %rax from the mapping variable's stack slot.
	   * Do not update descriptors. This is call scratch.
	   */
	  symEntry *target_entry = cur.arg1.val.sym_entry;

	  AsmOp fp_src = RBP_offset(target_entry->offset);
	  fp_src.ss = Q;

	  AsmOp fp_dst = RAX;
	  fp_dst.ss = Q;

	  asm_emit_mov_ss(fp_dst, fp_src, Q);     // movq offset(%rbp), %rax

	  /*
	   * Indirect call through %rax.
	   */
	  asm_emit_call_indirect(RAX);            // call *%rax

	  asm_emit_add_ss(RSP, src1, Q);

	  /* process return value */
	  int ret_ss = get_size_ss(cur.result.val.sym_entry);

	  dst.kind = ASM_OP_REG;
	  dst.val = get_reg1(cur);
	  dst.ss = ret_ss;

	  src1 = RAX;
	  src1.ss = ret_ss;

	  asm_emit_mov_ss(dst, src1, ret_ss);

	  cur.result.val.sym_entry->not_in_memory = true;

	  arg_n = 0;
	  break;
	}
        case IR_RETURN:

            /* move return value into return register (%rax) */
            if (cur.arg1.kind == IR_OP_SYMENTRY) {
	        int ss = get_size_ss(cur.arg1.val.sym_entry);
	        dst.kind = ASM_OP_REG;
		dst.val = 12;
		dst.ss = ss;
                src1.kind = ASM_OP_REG;
		src1.val = get_reg1_arg1(cur);
		src1.ss = ss;
                asm_emit_mov_ss(dst, src1,ss);
            }

            /* move contents of base pointer into stack pointer */
            dst = RSP;
            src1 = RBP;
            asm_emit_mov(dst, src1);

            /* pop base pointer */
            dst = RBP;
            asm_emit_pop(dst);

            /* return from function */
            asm_emit_ret();

            break;

        case IR_SEG_FAULT:
            /* load from address 0x0 to cause seg fault */
            dst = RAX;
            src1.kind = ASM_OP_IMM;
            src1.val = 0;
            asm_emit_mov_ss(dst, src1, Q);
            src1 = RAX;
            src1.kind = ASM_OP_OFFSET;
            src1.offset = 0;
            asm_emit_mov_ss(dst, src1, Q);
            break;

	case IR_ARRAY_ACCESS: {
	  /*
	    get_reg3's current convention:

	    regs.result = register holding arg1/base pointer
	    regs.arg2   = register holding arg2/byte offset
	    regs.arg1   = register chosen for result destination
	  */
	  reg3 regs = get_reg3(cur);

	  // Hotfix descriptor truth after get_reg3's weird bookkeeping.
	  assign_to_reg(regs.result, cur.arg1.val.sym_entry, true);
	  assign_to_reg(regs.arg2,   cur.arg2.val.sym_entry, true);

	  AsmOp base;
	  base.kind = ASM_OP_REG;
	  base.val = regs.result;
	  base.ss = Q;
    
	  AsmOp dst;
	  dst.kind = ASM_OP_REG;
	  dst.val = regs.arg1;
	  dst.ss = Q;
	  AsmOp off;
	  off.kind = ASM_OP_REG;
	  off.val = regs.arg2;
	  off.ss = Q;
	  // dst = base
	  asm_emit_mov_ss(dst, base, Q);

	  // dst = base + offset
	  asm_emit_addQ(dst, off);

	  // dst = *(base + offset)
	  AsmOp mem = {
	    .kind = ASM_OP_OFFSET,
	    .val = regs.arg1,
	    .offset = 0
	  };
	  AsmOp newDst;
	  newDst.kind = ASM_OP_REG;
	  newDst.val = regs.arg1;
	  newDst.ss = get_size_ss(cur.result.val.sym_entry);
	  asm_emit_mov_ss(newDst, mem, get_size_ss(cur.result.val.sym_entry));

	  assign_to_reg(regs.arg1, cur.result.val.sym_entry, true);
	  cur.result.val.sym_entry->reg_num = regs.arg1;
	  cur.result.val.sym_entry->not_in_memory = true;

	  break;
	}
        case IR_ARRAY_ASSIGN: {
	  regs3 = get_reg3Clean(cur);

	  AsmOp base = {.kind = ASM_OP_REG, .val = regs3.result, .ss = Q};
	  AsmOp idx  = {.kind = ASM_OP_REG, .val = regs3.arg1,   .ss = Q};
	  AsmOp val  = {.kind = ASM_OP_REG, .val = regs3.arg2,   .ss = get_size_ss(cur.arg2.val.sym_entry)};

	  asm_emit_add_ss(base, idx, Q);
	  symEntry *valueEntry = cur.arg2.val.sym_entry;
	  AsmOp mem = {
            .kind = ASM_OP_OFFSET,
            .val = base.val,
            .offset = 0,
            .ss = get_size_ss(valueEntry)
          };
	  asm_emit_mov_ss(mem, val, get_size_ss(valueEntry));
	  break;
	}
        default:
            asm_emit_nop();
            break;
        }
	 if (ir_array[i+1].is_leader && hasntSpilled) {
	  if (cur.op != IR_RETURN){
	  spill_all_regs();
	  }
	  else{
	    shouldSpill = false;
	  }
	 }
		
    }
    if(shouldSpill){
    spill_all_regs();
    }
    AsmOp l = {.kind = ASM_OP_LABEL, .val = i};
    asm_emit_label(l);

    //adding this since an unresolved jump happens if theres a jump to eof
    asm_emit_nop();
    asm_emit_end();
    asm_array_print(cg_file);
    return 0;
}

char* asm_op_as_str(AsmOp asmop) {
    char* b;
    switch (asmop.kind) {
    case ASM_OP_REG:
      switch(asmop.ss){
      case Q:
	return str_regs[asmop.val];
      case L:
	return strL_regs[asmop.val];
      case B:
	return strB_regs[asmop.val];
      }
      //return str_regs[asmop.val];
    case ASM_OP_IMM:
        b = malloc(20*sizeof(char));
        sprintf(b, "$%d", asmop.val);
        return b;
    case ASM_OP_OFFSET:
        b = malloc(30*sizeof(char));
        sprintf(b, "%d(%s)", asmop.offset, str_regs[asmop.val]);
        return b;
    case ASM_OP_LABEL:
        b = malloc(30*sizeof(char));
        sprintf(b, ".L%d", asmop.val);
        return b;
    case ASM_OP_FUNC_LABEL:
        if (asmop.func_entry != NULL) {
            b = strdup(asmop.func_entry->name);
        }
        return b;
    default:
        return "this shouldnt be printed";
    }
}

void asm_array_print(FILE* printloc) {
    //print preamble
    fprintf(printloc, "    .text\n");
    fprintf(printloc, "    .globl entry\n");
    fprintf(printloc, "    .type entry, @function\n");


    for (int i = 0; asm_array[i].op != ASM_END && i <= 4096; i++) {
        AsmInstr instr = asm_array[i];
        char* src = asm_op_as_str(instr.src);
        char* dst = asm_op_as_str(instr.dst);
        switch (instr.op) {
        case ASM_MOV:
            fprintf(printloc, "    mov%s %s, %s\n", size_suffixes[instr.ss], src, dst);
            break;
	case ASM_LEAQ_FUNC:
	  fprintf(printloc, "    leaq %s(%%rip), %s\n", instr.src.func_entry->name,dst);
	  break;
        case ASM_ADD:
            fprintf(printloc, "    add%s %s, %s\n", size_suffixes[instr.ss], src, dst);
            break;
        case ASM_SUB:
            fprintf(printloc, "    sub%s %s, %s\n", size_suffixes[instr.ss], src, dst);
            break;
        case ASM_IMUL:
            fprintf(printloc, "    imul%s %s, %s\n", size_suffixes[instr.ss], src, dst);
            break;
        case ASM_IDIV:
            fprintf(printloc, "    idiv%s %s\n", size_suffixes[instr.ss], src);
            break;
        case ASM_CLTD:
            fprintf(printloc, "    cltd\n");
            break;
        case ASM_NEG:
            fprintf(printloc, "    neg%s %s\n", size_suffixes[instr.ss], dst);
            break;
        case ASM_NOT:
            fprintf(printloc, "    not%s %s\n", size_suffixes[instr.ss], dst);
            break;
        case ASM_AND:
            fprintf(printloc, "    and%s %s, %s\n", size_suffixes[instr.ss], src, dst);
            break;
        case ASM_OR:
            fprintf(printloc, "    or%s %s, %s\n", size_suffixes[instr.ss], src, dst);
            break;
        case ASM_CMP:
            fprintf(printloc, "    cmp%s %s, %s\n", size_suffixes[instr.ss], src, dst);
            break;
        case ASM_JMP:
            fprintf(printloc, "    jmp %s\n", dst);
            break;
        case ASM_JL:
            fprintf(printloc, "    jl %s\n", dst);
            break;
        case ASM_JE:
            fprintf(printloc, "    je %s\n", dst);
            break;
        case ASM_PUSH:
            fprintf(printloc, "    pushq %s\n", src); //params are always 8byte for ease
            break;
        case ASM_POP:
            fprintf(printloc, "    popq %s\n", dst); //params are always 8byte for ease
            break;
        case ASM_CALL:
            fprintf(printloc, "    call %s\n", dst);
            break;
	case ASM_CALL_INDIRECT:
	  fprintf(printloc, "    call *%s\n", dst);
	  break;	    
        case ASM_RET:
            fprintf(printloc, "    ret\n");
            break;
        case ASM_LABEL:
            fprintf(printloc, "%s:\n", dst);
            break;
        case ASM_CMOVE:
            fprintf(printloc, "    cmove %s, %s\n", src, dst);
            break;
        case ASM_CMOVL:
            fprintf(printloc, "    cmovl %s, %s\n", src, dst);
            break;
        case ASM_NOP:
            fprintf(printloc, "    nop\n");
            break;
        default:
            return;
        }
    }
}

//emission functions
void asm_emit(AsmInstr a) {
    asm_array[asm_array_idx] = a;
    asm_array_idx++;
}

void asm_emit_end() {
    AsmInstr a = {.op = ASM_END};
    asm_emit(a);
}
void asm_emit_leaq_func(AsmOp dst, symEntry *fn_entry) {
    AsmOp src = {
        .kind = ASM_OP_FUNC_LABEL,
        .val = 0,
        .offset = 0,
        .ss = Q,
        .func_entry = fn_entry
    };

    AsmInstr a = {
        .op = ASM_LEAQ_FUNC,
        .dst = dst,
        .src = src,
        .ss = Q
    };

    asm_emit(a);
}
// the 'ss' variation is for moves with a size specifier Q, B or L
void asm_emit_mov_ss(AsmOp dst, AsmOp src, int ss) {
    AsmInstr a = {.op = ASM_MOV, .dst = dst, .src = src, .ss = ss};
    asm_emit(a);
}

void asm_emit_mov(AsmOp dst, AsmOp src) {
    AsmInstr a = {.op = ASM_MOV, .dst = dst, .src = src};
    asm_emit(a);
}

void asm_emit_add(AsmOp dst, AsmOp src) {
    AsmInstr a = {.op = ASM_ADD, .dst = dst, .src = src, .ss=L};
    asm_emit(a);
}
void asm_emit_addQ(AsmOp dst, AsmOp src) {
    AsmInstr a = {.op = ASM_ADD, .dst = dst, .src = src, .ss=Q};
    asm_emit(a);
}
void asm_emit_sub(AsmOp dst, AsmOp src) {
    AsmInstr a = {.op = ASM_SUB, .dst = dst, .src = src, .ss=L};
    asm_emit(a);
}

// the 'ss' variation is for moves with a size specifier Q, B or L
void asm_emit_sub_ss(AsmOp dst, AsmOp src, int ss) {
    AsmInstr a = {.op = ASM_SUB, .dst = dst, .src = src, .ss = ss};
    asm_emit(a);
}
void asm_emit_add_ss(AsmOp dst, AsmOp src, int ss) {
    AsmInstr a = {.op = ASM_ADD, .dst = dst, .src = src, .ss = ss};
    asm_emit(a);
}

void asm_emit_imul(AsmOp dst, AsmOp src) {
    AsmInstr a = {.op = ASM_IMUL, .dst = dst, .src = src, .ss=L};
    asm_emit(a);
}

void asm_emit_idiv(AsmOp src) {
    AsmInstr a = {.op = ASM_IDIV, .src = src, .ss=L};
    asm_emit(a);
}

void asm_emit_cltd() {
    AsmInstr a = {.op = ASM_CLTD};
    asm_emit(a);
}

void asm_emit_neg(AsmOp dst) {
    AsmInstr a = {.op = ASM_NEG, .dst = dst, .ss=L};
    asm_emit(a);
}

void asm_emit_not(AsmOp dst) {
    AsmInstr a = {.op = ASM_NOT, .dst = dst, .ss=B};
    asm_emit(a);
}

void asm_emit_and(AsmOp dst, AsmOp src) {
    AsmInstr a = {.op = ASM_AND, .dst = dst, .src = src, .ss=B};
    asm_emit(a);
}

void asm_emit_or(AsmOp dst, AsmOp src) {
    AsmInstr a = {.op = ASM_OR, .dst = dst, .src = src, .ss=B};
    asm_emit(a);
}

void asm_emit_cmp(AsmOp dst, AsmOp src) {
    AsmInstr a = {.op = ASM_CMP, .dst = dst, .src = src, .ss=B};
    asm_emit(a);
}
void asm_emit_cmp_ss(AsmOp dst, AsmOp src, int ss) {
    AsmInstr a = {.op = ASM_CMP, .dst = dst, .src = src, .ss=ss};
    asm_emit(a);
}

void asm_emit_jmp(AsmOp dst) {
    AsmInstr a = {.op = ASM_JMP, .dst = dst};
    asm_emit(a);
}

void asm_emit_jl(AsmOp dst) {
    AsmInstr a = {.op = ASM_JL, .dst = dst};
    asm_emit(a);
}

void asm_emit_je(AsmOp dst) {
    AsmInstr a = {.op = ASM_JE, .dst = dst};
    asm_emit(a);
}

void asm_emit_push(AsmOp src) {
    AsmInstr a = {.op = ASM_PUSH, .src = src, .ss=Q};
    asm_emit(a);
}

void asm_emit_pop(AsmOp dst) {
    AsmInstr a = {.op = ASM_POP, .dst = dst, .ss=Q};
    asm_emit(a);
}

void asm_emit_call(AsmOp dst) {
    AsmInstr a = {.op = ASM_CALL, .dst = dst};
    asm_emit(a);
}
void asm_emit_call_indirect(AsmOp dst) {
    AsmInstr a = {
        .op = ASM_CALL_INDIRECT,
        .dst = dst
    };
    asm_emit(a);
}
void asm_emit_ret() {
    AsmInstr a = {.op = ASM_RET};
    asm_emit(a);
}

void asm_emit_label(AsmOp dst) {
    AsmInstr a = {.op = ASM_LABEL, .dst = dst};
    asm_emit(a);
}

void asm_emit_cmove(AsmOp dst, AsmOp src) {
    AsmInstr a = {.op = ASM_CMOVE, .dst = dst, .src = src};
    asm_emit(a);
}

void asm_emit_cmovl(AsmOp dst, AsmOp src) {
    AsmInstr a = {.op = ASM_CMOVL, .dst = dst, .src = src};
    asm_emit(a);
}

void asm_emit_nop() {
    AsmInstr a = {.op = ASM_NOP};
    asm_emit(a);
}

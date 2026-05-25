#include <stdio.h>
#include "../src/ir.h"

int main() {
    // test that we can create an operand
    IrOp op = ir_op_from_int(42);

    // test that we can create an instruction
    IrInstr instr;
    instr.op = IR_ASSIGN;
    instr.result = op;
    instr.arg1 = op;
    instr.arg2 = op;

    printf("IR structure test passed\n");
    return 0;
}
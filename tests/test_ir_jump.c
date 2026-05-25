#include <stdio.h>
#include "../src/ir.h"

int main() {
    // test goto and if_false for a while loop
    IrOp cond = ir_op_from_bool(true);

    // emit iffalse cond goto <backpatch>
    ir_emit_if_false_bp(cond);

    // emit goto back to top (index 0)
    IrOp top = ir_op_from_ir_array_entry(0);
    ir_emit_goto(top);

    // backpatch the iffalse to jump to index 2 (after loop)
    BPList* bp = newBPList(0);
    backpatch(bp, 2);

    ir_emit_end();

    printf("IR jump test:\n");
    ir_array_print(stdout);
    return 0;
}
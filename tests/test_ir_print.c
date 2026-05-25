#include "../src/ir.h"
#include "stdio.h"
extern IrInstr ir_array[];
extern int ir_array_idx;

int main() {
    IrOp o1 = ir_op_from_ir_array_entry(1);
    IrOp o2 = ir_op_from_bool(true);
    IrOp o3 = ir_op_from_int(100);
    ir_emit_goto(o1);
    ir_emit_if_true_bp(o2);
    ir_emit_end();
    ir_array_print(stdout);
    return 0;
}

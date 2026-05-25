#include <stdio.h>
#include "../src/ir.h"

int main() {
    // test a simple assignment: x = 5
    IrOp result = ir_op_from_int(0);  // placeholder for x
    IrOp arg1 = ir_op_from_int(5);
    ir_emit_assign(result, arg1);
    ir_emit_end();

    printf("Expected: [0]: <none> = 5\n");
    printf("Got:      ");
    ir_array_print(stdout);
    return 0;
}
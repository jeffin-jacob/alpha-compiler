#include <stdio.h>
#include "../src/ir.h"

int main() {
    // test x = y + z
    IrOp x = ir_op_from_int(0);
    IrOp y = ir_op_from_int(3);
    IrOp z = ir_op_from_int(4);
    ir_emit_add(x, y, z);

    // test x = y * z
    ir_emit_mul(x, y, z);

    ir_emit_end();

    printf("Expected:\n");
    printf("[0]: <none> = 3 + 4\n");
    printf("[1]: <none> = 3 * 4\n");
    printf("Got:\n");
    ir_array_print(stdout);
    return 0;
}
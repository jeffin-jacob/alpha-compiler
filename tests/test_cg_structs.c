#include "../src/backend.h"
#include <string.h>

extern AsmInstr asm_array[];

int test1() {
    AsmOp x;
    x.kind = ASM_OP_REG;
    x.val = 1;
    printf("%s\n", asm_op_as_str(x));

    x.kind = ASM_OP_IMM;
    x.val = 21;
    printf("%s\n", asm_op_as_str(x));

    x.kind = ASM_OP_OFFSET;
    x.val = 10;
    x.offset = -5;
    printf("%s\n", asm_op_as_str(x));
}

int test2() {
    AsmOp x;
    x.kind = ASM_OP_REG;
    x.val = 1;

    AsmOp y;
    y.kind = ASM_OP_REG;
    y.val = 2;

    AsmInstr i;
    i.src = x;
    i.dst = y;
    i.op = ASM_MOV;

    asm_array[0] = i;

    i.op = ASM_END;
    asm_array[1] = i;
    asm_array_print(stdout);
}

int test3() {
    AsmOp x;
    x.kind = ASM_OP_REG;
    x.val = 1;

    AsmOp y;
    y.kind = ASM_OP_REG;
    y.val = 2;

    AsmInstr i;
    i.src = x;
    i.dst = y;
    i.ss = B;
    i.op = ASM_ADD;

    asm_array[0] = i;

    x.kind = ASM_OP_LABEL;
    x.val = 1;
    i.dst = x;
    i.op = ASM_LABEL;

    asm_array[1] = i;

    x.kind = ASM_OP_LABEL;
    x.val = 1;
    i.dst = x;
    i.op = ASM_JMP;

    asm_array[2] = i;

    i.op = ASM_END;
    asm_array[3] = i;
    asm_array_print(stdout);
}

int main(int argc, char **argv) {
    if (strcmp(argv[1], "1") == 0) {
        test1();
    }
    else if (strcmp(argv[1], "2") == 0) {
        test2();
    }
    else if (strcmp(argv[1], "3") == 0) {
        test3();
    }
}

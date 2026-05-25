#ifndef AST_TEST_H
#define AST_TEST_H

//Emir's test 'tokens'
//For each 'branch' of the grammar i'm writing a pseudotoken 
//here for testing
#define UNARY_NEG 901
#define BINARY_SUB 902

#define EXPR_CONST 903
#define EXPR_NEG 904
#define EXPR_NOT 905
#define EXPR_ADD 906
#define EXPR_SUB 907
#define EXPR_MUL 908
#define EXPR_DIV 909
#define EXPR_REM 910
#define EXPR_AND 911
#define EXPR_OR 912
#define EXPR_LT 913
#define EXPR_EQ 914
#define EXPR_ASSIGNABLE 915
#define EXPR_BINARY 916
#define EXPR_PAREN 917
#define EXPR_MEMOP 918

#define ARGLIST_MORE 919
#define ARGLIST_EXPR 920

#define ABLOCK 921

#define ASSIGNABLE_ABLOCK 922
#define ASSIGNABLE_RECOP 923

#define EXPR_LINE 924
#endif

#ifndef TOKENS_H
#define TOKENS_H

/*
 * Adapted from Section 1.2 of "The Alpha Programming Language" by
 * Carl Alphonce
 */

// identifier
#define ID          101

// type names
#define T_INTEGER   201
#define T_ADDRESS   202
#define T_BOOLEAN   203
#define T_CHARACTER 204
#define T_STRING    205

// constants (literals)
#define C_INTEGER   301
#define C_NULL      302
#define C_CHARACTER 303
#define C_STRING    304
#define C_TRUE      305
#define C_FALSE     306

// other keywords
#define WHILE       401
#define IF          402
#define THEN        403
#define ELSE        404
#define TYPE        405
#define FUNCTION    406
#define RETURN      407
#define EXTERNAL    408
#define AS          409

// punctuation - grouping
#define L_PAREN     501
#define R_PAREN     502
#define L_BRACKET   503
#define R_BRACKET   504
#define L_BRACE     505
#define R_BRACE     506
// punctuation - other
#define SEMI_COLON  507
#define COLON       508
#define COMMA       509
#define ARROW       510

// operators
#define ADD         601
#define SUB_OR_NEG  602
#define MUL         603
#define DIV         604
#define REM         605
#define LESS_THAN   606
#define EQUAL_TO    607
#define ASSIGN      608
#define NOT         609
#define AND         610
#define OR          611
#define DOT         612
#define RESERVE     613
#define RELEASE     614

// erroneous tokens
#define FD_UP_STRING 701

//added for comments
#define COMMENT     700
#define COMMENT_EOF 702
//added for now, it is what it is
#define UNDEFINED_INPUT 800

#endif

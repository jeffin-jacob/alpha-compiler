#include "../src/tokens.h"
#include "stdio.h"

extern int yylex();
extern FILE* yyin;


int test_punctuation1() {
    FILE *f = fopen("tests/data/punc1.txt", "r");
    if (f==NULL) {
        printf("FILE DOESNT EXIST\n");
        return 1;
    }
    yyin = f;

    int t = yylex();
    if (t != L_PAREN) {
        printf("EXPECTED L_PAREN(501) GOT: %i\n", t);
        return 1;
    }
    

    fclose(yyin);
    return 0;
}

int test_punctuation2() {
    FILE *f = fopen("tests/data/punc2.txt", "r");
    if (f==NULL) {
        printf("FILE DOESNT EXIST\n");
        return 1;
    }
    yyin = f;

    int t = yylex();
    if (t != L_PAREN) {
        printf("EXPECTED L_PAREN GOT: %i\n", t);
        return 1;
    }
    
    t = yylex();
    if (t != R_PAREN) {
        printf("EXPECTED R_PAREN GOT: %i\n", t);
        return 1;
    }
    
    t = yylex();
    if (t != L_BRACKET) {
        printf("EXPECTED L_BRACKET GOT: %i\n", t);
        return 1;
    }

    t = yylex();
    if (t != R_BRACKET) {
        printf("EXPECTED R_BRACKET GOT: %i\n", t);
        return 1;
    }

    t = yylex();
    if (t != L_BRACE) {
        printf("EXPECTED L_BRACE GOT: %i\n", t);
        return 1;
    }

    t = yylex();
    if (t != R_BRACE) {
        printf("EXPECTED R_BRACE GOT: %i\n", t);
        return 1;
    }

    t = yylex();
    if (t != COLON) {
        printf("EXPECTED COLON GOT: %i\n", t);
        return 1;
    }

    t = yylex();
    if (t != SEMI_COLON) {
        printf("EXPECTED SEMI_COLON GOT: %i\n", t);
        return 1;
    }

    t = yylex();
    if (t != COMMA) {
        printf("EXPECTED COMMA GOT: %i\n", t);
        return 1;
    }

    t = yylex();
    if (t != ARROW) {
        printf("EXPECTED ARROW GOT: %i\n", t);
        return 1;
    }

    fclose(yyin);
    return 0;
}


int test_string1() {
    FILE *f = fopen("tests/data/string1.txt", "r");
    if (f==NULL) {
        printf("FILE DOESNT EXIST\n");
        return 1;
    }
    yyin = f;

    int t = yylex();
    if (t != C_STRING) {
        printf("EXPECTED C_STRING GOT: %i\n", t);
        return 1;
    }
    
    t = yylex();
    if (t != SEMI_COLON) {
        printf("EXPECTED SEMI_COLON GOT: %i\n", t);
        return 1;
    }

    fclose(yyin);
    return 0;
}

int test_string2() {
    FILE *f = fopen("tests/data/string2.txt", "r");
    if (f==NULL) {
        printf("FILE DOESNT EXIST\n");
        return 1;
    }
    yyin = f;

    int t = yylex();
    if (t != C_STRING) {
        printf("EXPECTED C_STRING GOT: %i\n", t);
        return 1;
    }
    
    t = yylex();
    if (t != SEMI_COLON) {
        printf("EXPECTED SEMI_COLON GOT: %i\n", t);
        return 1;
    }

    fclose(yyin);
    return 0;
}

int test_string5() {
    FILE *f = fopen("tests/data/string5NEW.txt", "r");
    if (f==NULL) {
        printf("FILE DOESNT EXIST\n");
        return 1;
    }
    yyin = f;

    int t = yylex();
    if (t != ID) {
        printf("SINCE THERE IS A SINGLE QUOTE ON LINE THIS SHOULD BE ID: %i\n", t);
        return 1;
    }

    t = yylex();
    if (t != T_STRING) {
        printf("SINCE THERE IS A SINGLE QUOTE ON LINE THIS SHOULD BE T_STR: %i\n", t);
        return 1;
    }

    fclose(yyin);
    return 0;
}


int test_string4() {
    FILE *f = fopen("tests/data/string4.txt", "r");
    if (f==NULL) {
        printf("FILE DOESNT EXIST\n");
        return 1;
    }
    yyin = f;

    int t = yylex();
    if (t != C_STRING) {
        printf("EXPECTED STRING GOT: %i\n", t);
        return 1;
    }
    
    t = yylex();
    if (t != SEMI_COLON) {
        printf("EXPECTED SEMI_COLON GOT: %i\n", t);
        return 1;
    }
    
    
    fclose(yyin);
    return 0;
}

int main() {
    printf("testing basic punctuation: ");
    if (test_punctuation1()) {
        printf("FAILED\n");
    }
    else {
        printf("PASSED\n");
    }

    printf("testing all punctuation: ");
    if (test_punctuation2()) {
        printf("FAILED\n");
    }
    else {
        printf("PASSED\n");
    }

    printf("testing basic string literal: ");
    if (test_string1()) {
        printf("FAILED\n");
    }
    else {
        printf("PASSED\n");
    }

    printf("testing string literal with symbols and escaped quotes: ");
    if (test_string2()) {
        printf("FAILED\n");
    }
    else {
        printf("PASSED\n");
    }

    printf("testing string with multiple escape sequences: ");
    if (test_string4()) {
        printf("FAILED\n");
    }
    else {
        printf("PASSED\n");
    }

    printf("testing invalid string NEW PROPER REGEX: ");
    if (test_string5()) {
        printf("FAILED\n");
    }
    else {
        printf("PASSED\n");
    }


    return 0;
}

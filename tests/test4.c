#include "../src/tokens.h"
#include <stdio.h>

char *test_name;

void print_error(char *macro, int value) {
    printf("%s: %s <> %d\n", test_name, macro, value);
}

int main(int argc, char *argv[]) {
    test_name = argv[0];
    int errors = 0;
    if (ID != 101) {
        errors++;
        print_error("ID", 101);
    }
    if (T_INTEGER != 201) {
        errors++;
        print_error("T_INTEGER", 201);
    }
    if (T_ADDRESS != 202) {
        errors++;
        print_error("T_ADDRESS", 202);
    }
    if (T_BOOLEAN != 203) {
        errors++;
        print_error("T_BOOLEAN", 203);
    }
    if (T_CHARACTER != 204) {
        errors++;
        print_error("T_CHARACTER", 204);
    }
    if (T_STRING != 205) {
        errors++;
        print_error("T_STRING", 205);
    }
    if (C_NULL != 302) {
        errors++;
        print_error("C_NULL", 302);
    }
    if (C_TRUE != 305) {
        errors++;
        print_error("C_TRUE", 305);
    }
    if (C_FALSE != 306) {
        errors++;
        print_error("C_FALSE", 306);
    }
    if (WHILE != 401) {
        errors++;
        print_error("WHILE", 401);
    }
    if (IF != 402) {
        errors++;
        print_error("IF", 402);
    }
    if (THEN != 403) {
        errors++;
        print_error("THEN", 403);
    }
    if (ELSE != 404) {
        errors++;
        print_error("ELSE", 404);
    }
    if (TYPE != 405) {
        errors++;
        print_error("TYPE", 405);
    }
    if (FUNCTION != 406) {
        errors++;
        print_error("FUNCTION", 406);
    }
    if (RETURN != 407) {
        errors++;
        print_error("RETURN", 407);
    }
    if (EXTERNAL != 408) {
        errors++;
        print_error("EXTERNAL", 408);
    }
    if (AS != 409) {
        errors++;
        print_error("AS", 409);
    }
    if (errors) {
        printf("%s: %d error(s). Test Failed\n", test_name, errors);
        return 1;
    }
    printf("%s: Test Passed\n", test_name);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/tokens.h"
#include "../src/ast_test_defs.h"

#define MAX_LENGTH 1000

char *test_name;
extern int yydebug;

extern FILE *yyin;
extern int yyparse();

int main(int argc, char *argv[]) {
    yydebug = 1;

    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        fprintf(stderr, "%s: Unable to read\n", argv[1]);
        return 1;
    }
    test_name = argv[0];
    
    yyparse();
    return 0;
}

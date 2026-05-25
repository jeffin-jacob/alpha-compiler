#include <stdio.h>

#define INPUT_FILE "tests/data/input17.a"

extern FILE *yyin;
extern int yyparse();

int main() {
    yyin = fopen(INPUT_FILE, "r");
    if (yyin == NULL) {
        fprintf(stderr, "%s: Unable to read\n", INPUT_FILE);
        return 1;
    }
    yyparse();
    return 0;
}

#include "../src/tokens.h"
#include "stdio.h"

extern int yylex();
extern FILE *yyin;

int main() {
    yylex();
}

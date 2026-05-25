/* #include "../src/ast_test_defs.h" */
/* #include "../src/tokens.h" */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/symbol-table.h"

#define MAX_LENGTH 1000

char *test_name;

extern FILE *yyin;
extern int yyparse();
extern symbolTable *headTable;

FILE *asc_file = NULL;
int main(int argc, char *argv[]) {

    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        fprintf(stderr, "%s: Unable to read\n", argv[1]);
        return 1;
    }

    newSymbolTable(NULL, 1, 1);
    initializeGlobalSymbols();

    // manually add mapping type to symbol table to test param
    symEntry *sym_int = getEntryInSymbolTable(headTable, "integer", true);
    typeExpr *type_fn = type_mapping(sym_int, sym_int);
    symEntry *sym_fn = sym_make_function("identity", type_fn);
    addEntry(headTable, sym_fn);

    yyparse();
    print_symbol_table(headTable, stdout);
    return 0;
}

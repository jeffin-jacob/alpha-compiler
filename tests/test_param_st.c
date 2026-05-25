#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/tokens.h"
#include "../src/ast_test_defs.h"

#include "../src/symbol-table.h"

#define MAX_LENGTH 1000

char *test_name;

extern FILE *yyin;
extern int yyparse();
extern symbolTable* headTable;

FILE* asc_file = NULL;
int main(int argc, char *argv[]) {

    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        fprintf(stderr, "%s: Unable to read\n", argv[1]);
        return 1;
    }

    newSymbolTable(NULL, 1, 1);
    initializeGlobalSymbols();

    //manually add mapping type to symbol table to test param
    symEntry* sym_int = getEntryInSymbolTable(headTable, "integer", true);
    typeExpr* type_fn = type_mapping(sym_int, sym_int);
    symEntry* sym_fn = sym_make_function("square", type_fn);
    addEntry(headTable, sym_fn);

    //manually add rec to test param
    symbolTable* r = newSymbolTableForRecord();

    typeExpr* type_a = type_single(sym_int);
    symEntry* sym_a = sym_make_local("a", type_a);
    addEntry(r, sym_a);

    typeExpr* type_b = type_single(sym_int);
    symEntry* sym_b = sym_make_local("b", type_b);
    addEntry(r, sym_b);

    typeExpr* type_rec = type_record(2, r);
    symEntry* sym_rec = sym_make_type("bleh", type_rec);
    addEntry(headTable, sym_rec);

    typeExpr* type_fn2 = type_mapping(sym_rec, sym_int);
    symEntry* sym_fn2 = sym_make_function("mul", type_fn2);
    addEntry(headTable, sym_fn2);
    yyparse();
    print_symbol_table(headTable, stdout);
    return 0;
}

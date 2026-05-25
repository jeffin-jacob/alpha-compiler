#include "../src/symbol-table.h"
#include <string.h>
#include <stdio.h>

#define NUM_OF_TABLES 6

symbolTable *test_tables[NUM_OF_TABLES];

/* Initialize a symbol table tree for testing */
void setup() {
    test_tables[0] = newSymbolTable(NULL, 1, 1);
    test_tables[1] = newSymbolTable(test_tables[0], 2, 2);
    test_tables[2] = newSymbolTable(test_tables[1], 3, 3);
    test_tables[3] = newSymbolTable(test_tables[1], 4, 4);
    test_tables[4] = newSymbolTable(test_tables[2], 5, 5);
    test_tables[5] = newSymbolTable(test_tables[2], 6, 6);
}


void test1() {
    symbolTable *table1 = newSymbolTable(NULL, 1, 1);
    typeExpr *t = type_primitive();
    symEntry *set = sym_make_type("integer", t);
    addEntry(table1, set);

    print_symbol_table(table1, stdout);
}

void test2() {
    symbolTable *table1 = newSymbolTable(NULL, 1, 1);
    typeExpr *t = type_primitive();
    symEntry *set = sym_make_type("integer", t);
    addEntry(table1, set);

    typeExpr *t1 = type_single(set);
    symEntry *set1 = sym_make_local("userint", t1);
    addEntry(table1, set1);

    symbolTable *table2 = newSymbolTable(table1, 2, 2);

    typeExpr *t2 = type_single(set);
    symEntry *set2 = sym_make_local("test2", t2);
    addEntry(table2, set2);

    symbolTable *table3 = newSymbolTable(table1, 3, 3);

    typeExpr *t3 = type_single(set);
    symEntry *set3 = sym_make_local("testa", t3);
    addEntry(table3, set3);
    
    typeExpr *t4 = type_single(set);
    symEntry *set4 = sym_make_local("testb", t4);
    addEntry(table3, set4);

    print_symbol_table(table1, stdout);
}

void test3() {
    symbolTable *table1 = newSymbolTable(NULL, 1, 1);
    typeExpr *t = type_primitive();
    symEntry *set = sym_make_type("integer", t);
    addEntry(table1, set);
    
    typeExpr *i1 = type_single(set);

    typeExpr *t1 = type_mapping(set, set);
    symEntry *i2i = sym_make_type("int2int", t1);
    addEntry(table1, i2i);

    typeExpr *i2iType = type_single(i2i);
    symEntry *f = sym_make_function("square", i2iType);
    addEntry(table1, f);
    symbolTable *table2 = newSymbolTable(table1, 14, 14);

    symEntry *paramx = sym_make_param("x", i1, f);
    addEntry(table2, paramx);
    print_symbol_table(table1, stdout);
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

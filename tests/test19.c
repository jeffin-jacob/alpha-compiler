#include "../src/symbol-table.h"
#include <assert.h>
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

/* Symbol table and related functions */
void test1() {

    for (int i = 0; i < NUM_OF_TABLES; i++) {
        assert(test_tables[i] != NULL);
    }
    assert(getSymbolTable() == test_tables[0]);
    assert(getParent(test_tables[0]) == NULL);
    assert(getParent(test_tables[1]) == test_tables[0]);
    assert(getParent(test_tables[2]) == test_tables[1]);
    assert(getParent(test_tables[3]) == test_tables[1]);
    assert(getParent(test_tables[4]) == test_tables[2]);
    assert(getParent(test_tables[5]) == test_tables[2]);
}

/* Symbol table list and related functions */
void test2() {
    assert(getChildren(test_tables[0]) != NULL);
    assert(getChildren(test_tables[0])->elem == test_tables[1]);
    assert(getRestOfChildren(getChildren(test_tables[0])) == NULL);
    assert(getChildren(test_tables[1]) != NULL);
    assert(getChildren(test_tables[1])->elem == test_tables[2]);
    assert(getRestOfChildren(getChildren(test_tables[1])) != NULL);
    assert(getRestOfChildren(getChildren(test_tables[1]))->elem ==
           test_tables[3]);
    assert(getFirstOfChildren(getRestOfChildren(getChildren(test_tables[1]))) !=
           NULL);
    assert(getFirstOfChildren(getRestOfChildren(getChildren(test_tables[1]))) ==
           test_tables[2]);
    assert(getRestOfChildren(getRestOfChildren(getChildren(test_tables[1]))) ==
           NULL);
    assert(getChildren(test_tables[2]) != NULL);
    assert(getChildren(test_tables[2])->elem == test_tables[4]);
    assert(getRestOfChildren(getChildren(test_tables[2])) != NULL);
    assert(getRestOfChildren(getChildren(test_tables[2]))->elem ==
           test_tables[5]);
    assert(getFirstOfChildren(getRestOfChildren(getChildren(test_tables[2]))) !=
           NULL);
    assert(getFirstOfChildren(getRestOfChildren(getChildren(test_tables[2]))) ==
           test_tables[4]);
    assert(getRestOfChildren(getRestOfChildren(getChildren(test_tables[2]))) ==
           NULL);
    assert(getChildren(test_tables[3]) == NULL);
    assert(getRestOfChildren(getChildren(test_tables[3])) == NULL);
    assert(getChildren(test_tables[4]) == NULL);
    assert(getRestOfChildren(getChildren(test_tables[4])) == NULL);
    assert(getChildren(test_tables[5]) == NULL);
    assert(getRestOfChildren(getChildren(test_tables[5])) == NULL);
}

/* Finding with respect to entry and related functions */
void test3() {
    symEntry *needleEntry;
    test_tables[0]->entry = sym_make_local("haystack", type_undefined());
    test_tables[1]->entry =
        (needleEntry = sym_make_local("needle", type_undefined()));
    test_tables[2]->entry = sym_make_local("haystack", type_undefined());
    assert(getEntryInSymbolTable(test_tables[5], "needle", false) == NULL);
    assert(getEntryInSymbolTable(test_tables[5], "needle", true) ==
           needleEntry);
    assert(getSymbolTableWithEntry(needleEntry) == test_tables[1]);
}

int main(int argc, char *argv[]) {
    setup();
    test1();
    test2();
    test3();
    printf("%s: Tests Passed!\n", argv[0]);
    return 0;
}

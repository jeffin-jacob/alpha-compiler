#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "symEntry.h"
#include "stdio.h"
#include <stdbool.h>

typedef struct symbolTable symbolTable;

typedef struct symbolTableList symbolTableList;

struct symbolTable {
    int line;
    int col;
    symEntry *entry;
    symbolTable *parent;
    symbolTableList *children;
};

struct symbolTableList {
    symbolTable *elem;
    symbolTableList *next;
    symbolTableList *prev;
};

/* Constructor(s) */
symbolTable *newSymbolTable(symbolTable *parent, int line, int col);

symbolTable *newSymbolTableForRecord();

symbolTableList *newSymbolTableList(symbolTable *table);

/* Mutator(s) */
void addChildToSymbolTable(symbolTable *parent, symbolTable *child);

void initializeGlobalSymbols();
/* Accessor(s) */
symbolTable *getSymbolTable();

symbolTable *getParent(symbolTable *table);

symbolTableList *getChildren(symbolTable *table);

symbolTableList *getRestOfChildren(symbolTableList *list);

symbolTable *getFirstOfChildren(symbolTableList *list);

void addEntry(symbolTable *table, symEntry *e);

symEntry *getEntryInSymbolTable(symbolTable *table, char *name,
                                bool ancestorSearch);

symbolTable *getSymbolTableWithEntry(symEntry *entry);

symbolTable *get_function_table(symEntry *func);

void print_symbol_table(symbolTable *table, FILE *printloc);
#endif

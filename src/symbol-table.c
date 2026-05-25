#include "symbol-table.h"
#include "symEntry.h"
#include <stdlib.h>

symbolTable *headTable;
symbolTable *curTable;

symbolTable *newSymbolTable(symbolTable *parent, int line, int col) {
    symbolTable *child = malloc(sizeof(symbolTable));
    child->entry = NULL;
    child->children = NULL;
    child->line = line;
    child->col = col;
    addChildToSymbolTable(parent, child);
    curTable = child;
    return child;
}

symbolTable *newSymbolTableForRecord() {
    symbolTable *child = malloc(sizeof(symbolTable));
    child->entry = NULL;
    child->children = NULL;
    child->line = 0;
    child->col = 0;
    return child;
}

void initializeGlobalSymbols() {
    //initialize primitives added manually
    if (headTable == NULL) {
        return;
    }
    typeExpr* type_int = type_primitive(PRIM_INT);
    symEntry* sym_int = sym_make_type("integer", type_int);
    addEntry(headTable, sym_int);

    typeExpr* type_bool = type_primitive(PRIM_BOOL);
    symEntry* sym_bool = sym_make_type("Boolean", type_bool);
    addEntry(headTable, sym_bool);

    typeExpr* type_char = type_primitive(PRIM_CHAR);
    symEntry* sym_char = sym_make_type("character", type_char);
    addEntry(headTable, sym_char);

    typeExpr *type_string = type_array(sym_char, 1); 
    symEntry *sym_string = sym_make_type("string", type_string);
    addEntry(headTable, sym_string);
    
    typeExpr* type_addr = type_primitive(PRIM_ADDR);
    symEntry* sym_addr = sym_make_type("address", type_addr);
    addEntry(headTable, sym_addr);

    //added null literal for type checking
    typeExpr* type_null = type_primitive(PRIM_BOOL);
    symEntry* sym_null = sym_make_type("null", type_null);
    addEntry(headTable, sym_null);


    /* reserve and release functions */
    typeExpr *type_reserve = type_mapping(sym_int, sym_addr);
    symEntry *sym_reserve = sym_make_function("reserve", type_reserve);
    addEntry(headTable, sym_reserve);

    /* I don't know why there is a "null" type; so, I set the range of
     * release to address. You may want/need to change it to "null"
     * later. */
    typeExpr *type_release = type_mapping(sym_int, sym_addr);
    symEntry *sym_release = sym_make_function("release", type_release);
    addEntry(headTable, sym_release);

    //Adding hardcoded library functions in here
    //printInteger
    typeExpr *mtype_i2i = type_mapping(sym_int, sym_int);
    symEntry *sym_i2i = sym_make_type("<integer->integer>", mtype_i2i);
    typeExpr *stype_i2i = type_single(sym_i2i);
    symEntry *ssym_i2i = sym_make_type("integer2integer", stype_i2i);
    symEntry *sym_printint = sym_make_function("printInteger", stype_i2i);
    addEntry(headTable, ssym_i2i);
    addEntry(headTable, sym_printint);

    //printCharacter
    typeExpr *mtype_c2i = type_mapping(sym_char, sym_int);
    symEntry *sym_c2i = sym_make_type("<character->integer>", mtype_c2i);
    typeExpr *stype_c2i = type_single(sym_c2i);
    symEntry *ssym_c2i = sym_make_type("character2integer", stype_c2i);
    symEntry *sym_printchar = sym_make_function("printCharacter", stype_c2i);
    addEntry(headTable, ssym_c2i);
    addEntry(headTable, sym_printchar);

    //printBoolean
    typeExpr *mtype_b2i = type_mapping(sym_bool, sym_int);
    symEntry *sym_b2i = sym_make_type("<Boolean->integer>", mtype_b2i);
    typeExpr *stype_b2i = type_single(sym_b2i);
    symEntry *ssym_b2i = sym_make_type("boolean2integer", stype_b2i);
    symEntry *sym_printbool = sym_make_function("printBoolean", stype_b2i);
    addEntry(headTable, ssym_b2i);
    addEntry(headTable, sym_printbool);
}

symbolTableList *newSymbolTableList(symbolTable *table) {
    if (table == NULL) {
        return NULL;
    }
    symbolTableList *listP = malloc(sizeof(symbolTableList));
    listP->elem = table;
    listP->next = NULL;
    listP->prev = NULL;
    return listP;
}

void addEntry(symbolTable *table, symEntry *e) {
    if (table == NULL) {
        return;
    }
    if (table->entry == NULL) {
        table->entry = e;
        return;
    }
    entry_list_append(table->entry, e);
    return;
}


void addChildToSymbolTable(symbolTable *parent, symbolTable *child) {
    if (child != NULL) {
        child->parent = parent;
        if (parent != NULL) {
            symbolTableList *newTail = newSymbolTableList(child);
            if (parent->children != NULL) {
                symbolTableList *currNode = parent->children;
                while (currNode->next != NULL) {
                    currNode = currNode->next;
                }
                currNode->next = newTail;
                newTail->prev = currNode;
            } else {
                parent->children = newTail;
            }
        } else {
            headTable = child;
        }
    }
}

symbolTable *getSymbolTable() { return headTable; }

symbolTable *getParent(symbolTable *table) {
    if (table == NULL) {
        return NULL;
    }
    return table->parent;
}

symbolTableList *getChildren(symbolTable *table) {
    if (table == NULL) {
        return NULL;
    }
    return table->children;
}

symEntry *getEntries(symbolTable *table) {
    if (table == NULL) {
        return NULL;
    }
    return table->entry;
}

symbolTableList *getRestOfChildren(symbolTableList *list) {
    if (list == NULL) {
        return NULL;
    }
    return list->next;
}

symbolTable *getFirstOfChildren(symbolTableList *list) {
    if (list == NULL) {
        return NULL;
    }
    symbolTableList *currNode = list;
    while (currNode->prev != NULL) {
        currNode = currNode->prev;
    }
    return currNode->elem;
}

symEntry *getEntryInSymbolTable(symbolTable *table, char *name,
                                bool ancestorSearch) {
    if (table == NULL) {
        return NULL;
    }
    if (ancestorSearch) {
        symbolTable *curr_node = table;
        while (curr_node != NULL) {
            symEntry *entryP = entry_list_find(curr_node->entry, name);
            if (entryP != NULL) {
                return entryP;
            }
            curr_node = curr_node->parent;
        }
        return NULL;
    }
    return entry_list_find(table->entry, name);
}

/* Recursive DFS */
/* static symbolTable *dfs(symbolTable *start, symEntry *entry, int i) { */
/*     if (start == NULL) { */
/*         return NULL; */
/*     } */

/*     /1* process *1/ */

/*     symbolTableList *currDescendant = getChildren(start); */
/*     while (currDescendant != NULL) { */
/*         dfs(currDescendant->elem, entry, i + 1); */
/*         currDescendant = getRestOfChildren(currDescendant); */
/*     } */
/*     return NULL; */
/* } */

/* Iterative DFS */
/* Returns symbol table which satisifies a condition between two entries */
static symbolTable *dfs(symbolTable *start, symEntry *entry, 
                        bool (*cond_func)(symEntry *, symEntry *)) {
    symbolTableList *gc;
    symbolTableList *stack = newSymbolTableList(start);
    while (stack != NULL) {
        symbolTable *currNode = stack->elem;
        gc = stack;
        stack = stack->next;
        free(gc);

        /* process */
        if (cond_func(currNode->entry, entry)) {
            return currNode;
        }

        symbolTableList *currDescendant = getChildren(currNode);
        while (currDescendant != NULL) {
            symbolTableList *newStack =
                newSymbolTableList(currDescendant->elem);
            newStack->next = stack;
            stack = newStack;
            currDescendant = getRestOfChildren(currDescendant);
        }
    }
    return NULL;
}

symbolTable *getSymbolTableWithEntry(symEntry *entry) {
    return dfs(getSymbolTable(), entry, entry_list_hasInIt);
}

symbolTable *get_function_table(symEntry *func) {
    return dfs(getSymbolTable(), func, entry_list_has_param_of);
}

void print_annotation(symEntry *e, FILE *printloc) {
    if (e->ann == ANN_TYPE) {
        fprintf(printloc, "type");
    }
    else if (e->ann == ANN_FUNCTION) {
        fprintf(printloc, "function");
    }
    else if (e->ann == ANN_LOCAL) {
        fprintf(printloc, "local");
    }
    else if (e->ann == ANN_PARAMETER) {
        fprintf(printloc, "parameter of (%s)", e->paramOf->name);
    }

}

void print_type(typeExpr *t, FILE *printloc) {
    if (t->kind == TYPE_PRIMITIVE) {
        fprintf(printloc, "primitive");
    }
    else if (t->kind == TYPE_SINGLE) {
        fprintf(printloc, "%s", t->as.single->name);
    }
    else if (t->kind == TYPE_MAPPING) {
        fprintf(printloc, "%s->%s", t->as.map.dom->name, t->as.map.rng->name);
    }
    else if (t->kind == TYPE_RECORD) {
        fprintf(printloc, "record of (");
        symEntry* x = t->as.rec.scope->entry;
        while(true) {
            fprintf(printloc, "%s", x->name);
            x=x->next;
            if (x == NULL) {
                break;
            }
            fprintf(printloc, ",");
        }
        fprintf(printloc, ")");
    }
    else if (t->kind == TYPE_ARRAY) {
        fprintf(printloc, "%d->%s", t->as.arr.dim, t->as.arr.elem->name);
    }
    else if (t->kind == TYPE_UNDEFINED) {
        fprintf(printloc, "$_undefined_type");
    }
}

void print_symbol_table_rec(symbolTable *table, FILE *printloc) {
    fprintf(printloc, "------------------------------------------------------\n");
    symEntry *e = getEntries(table);
    while (e != NULL) {
        fprintf(printloc, "%s : %03d%03d : ", e->name, table->line, table->col);
        if (table->parent != NULL) {
            fprintf(printloc, "%03d%03d", table->parent->line, table->parent->col);
        }
        fprintf(printloc, " : ");
        print_type(e->type, printloc);
        fprintf(printloc, " : ");
        print_annotation(e, printloc);
        fprintf(printloc, " : %d ", e->offset);
        fprintf(printloc, "\n");
        e = e->next;
    }
    symbolTableList *s = getChildren(table);
    while (s != NULL) {
        print_symbol_table_rec(s->elem, printloc);
        s = s->next;
    }
}

void print_symbol_table(symbolTable *table, FILE *printloc) {
    fprintf(printloc, "NAME : SCOPE : PARENT : TYPE : EXTRA ANNOTATION : OFFSET\n");
    print_symbol_table_rec(table, printloc);
}


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stdbool.h>

#include "../src/symEntry.h" 



int single(void) {
    typeExpr *t_undef = type_undefined();
    symEntry *integer = sym_make_type("integer", t_undef);
    if (!integer) {
        printf("FAILED: sym_make_type(integer) returned NULL\n");
        return 1;
    }

    typeExpr *t = type_single(integer);
    if (!t) {
        printf("FAILED: type_single returned NULL\n");
        return 1;
    }
    if (t->kind != TYPE_SINGLE) {
        printf("FAILED: expected TYPE_SINGLE, got %d\n", (int)t->kind);
        return 1;
    }
    if (t->as.single != integer) {
        printf("FAILED: expected as.single to point to integer symEntry\n");
       
        return 1;
    }

    return 0;
}


int mapping(void) {
    symEntry *stringT  = sym_make_type("string",  type_undefined());
    symEntry *integerT = sym_make_type("integer", type_undefined());
    if (!stringT || !integerT) {
        printf("FAILED: could not create primitive type entries\n");
        return 1;
    }

    typeExpr *m = type_mapping(stringT, integerT);
    if (!m) {
        printf("FAILED: type_mapping returned NULL\n");
        return 1;
    }
    if (m->kind != TYPE_MAPPING) {
        printf("FAILED: expected TYPE_MAPPING, got %d\n", (int)m->kind);
        return 1;
    }
    if (m->as.map.dom != stringT || m->as.map.rng != integerT) {
        printf("FAILED: mapping dom/rng pointers incorrect\n");
       
        return 1;
    }

    return 0;
}
int array(void) {
    symEntry *integerT = sym_make_type("integer", type_undefined());
    if (!integerT) {
        printf("FAILED: could not create primitive type entries\n");
        return 1;
    }

    typeExpr *m = type_array(integerT,3);
    if (!m) {
        printf("FAILED: type_array returned NULL\n");
        return 1;
    }
    if (m->kind != TYPE_ARRAY) {
        printf("FAILED: expected TYPE_ARRAY, got %d\n", (int)m->kind);
        return 1;
    }
    if (m->as.arr.elem != integerT) {
        printf("FAILED: as.arr.elem pointer incorrect\n");

        return 1;
    }
    if (m->as.arr.dim != 3) {
        printf("FAILED: as.arr.dim value incorrect\n");

        return 1;
    }

    return 0;
}
int record(void) {
    // using dummy pointer to test for now
    symbolTable *fakeScope = (symbolTable*)0x1;

    typeExpr *r = type_record(fakeScope);
    if (!r) {
        printf("FAILED: type_record returned NULL\n");
        return 1;
    }
    if (r->kind != TYPE_RECORD) {
        printf("FAILED: expected TYPE_RECORD, got %d\n", (int)r->kind);
        return 1;
    }
    if (r->as.recordScope != fakeScope) {
        printf("FAILED: recordScope pointer incorrect\n");
       
        return 1;
    }

    return 0;
}

int undefined(void) {
    typeExpr *u = type_undefined();
    if (!u) {
        printf("FAILED: type_undefined returned NULL\n");
        return 1;
    }
    if (u->kind != TYPE_UNDEFINED) {
        printf("FAILED: expected TYPE_UNDEFINED, got %d\n", (int)u->kind);
        return 1;
    }
    return 0;
}



int wrappers(void) {
    symEntry *intType = sym_make_type("integer", type_undefined());
    if (!intType) {
        printf("FAILED: sym_make_type(integer) returned NULL\n");
        return 1;
    }

    symEntry *square = sym_make_function("square", type_undefined());
    if (!square) {
        printf("FAILED: sym_make_function(square) returned NULL\n");
        return 1;
    }
    if (square->ann != ANN_FUNCTION) {
        printf("FAILED: expected ANN_FUNCTION for square, got %d\n", (int)square->ann);
        return 1;
    }
    if (square->paramOf != NULL) {
        printf("FAILED: expected square->paramOf == NULL\n");
        return 1;
    }

    // parameter x : integer (of square)
    symEntry *x = sym_make_param("x", type_single(intType), square);
    if (!x) {
        printf("FAILED: sym_make_param(x) returned NULL\n");
        return 1;
    }
    if (x->ann != ANN_PARAMETER) {
        printf("FAILED: expected ANN_PARAMETER for x, got %d\n", (int)x->ann);
        return 1;
    }
    if (x->paramOf != square) {
        printf("FAILED: expected x->paramOf == square\n");
        return 1;
    }

    // local input : integer
    symEntry *input = sym_make_local("input", type_single(intType));
    if (!input) {
        printf("FAILED: sym_make_local(input) returned NULL\n");
        return 1;
    }
    if (input->ann != ANN_LOCAL) {
        printf("FAILED: expected ANN_LOCAL for input, got %d\n", (int)input->ann);
        return 1;
    }
    if (input->paramOf != NULL) {
        printf("FAILED: expected input->paramOf == NULL\n");
        return 1;
    }

    return 0;
}

// testing to see if I copied correctly
int nameCopy(void) {
    symEntry *intType = sym_make_type("integer", type_undefined());
    if (!intType) {
        printf("FAILED: could not create integer type\n");
        return 1;
    }

    char buf[64];
    strcpy(buf, "tempname");
    symEntry *e = sym_make_local(buf, type_single(intType));
    if (!e) {
        printf("FAILED: sym_make_local returned NULL\n");
        return 1;
    }

    strcpy(buf, "CHANGED");
    if (strcmp(e->name, "tempname") != 0) {
        printf("FAILED: expected deep-copied name 'tempname', got '%s'\n", e->name);
        return 1;
    }

    return 0;
}

//linked list test

int linkedListTest(void) {
    symEntry *intType = sym_make_type("integer", type_undefined());
    if (!intType) {
        printf("FAILED: could not create integer type\n");
        return 1;
    }

    symEntry *a = sym_make_local("a", type_single(intType));
    symEntry *b = sym_make_local("b", type_single(intType));
    symEntry *c = sym_make_local("c", type_single(intType));
    if (!a || !b || !c) {
        printf("FAILED: could not create list nodes a/b/c\n");
        return 1;
    }

    symEntry *head = a;

    entry_list_append(head, b);
    entry_list_append(head, c);

    // length should be 3
    int len = entry_list_length(head);
    if (len != 3) {
        printf("FAILED: expected length 3, got %d\n", len);
        return 1;
    }

    // find b, c
    symEntry *fb = entry_list_find(head, "b");
    if (fb != b) {
        printf("FAILED: entry_list_find('b') did not return b node\n");
        
        return 1;
    }

    symEntry *fc = entry_list_find(head, "c");
    if (fc != c) {
        printf("FAILED: entry_list_find('c') did not return c node\n");
        return 1;
    }

    symEntry *fz = entry_list_find(head, "nope");
    if (fz != NULL) {
        printf("FAILED: entry_list_find('nope') expected NULL\n");
        return 1;
    }
    bool checker = entry_list_hasInIt(head, fb);
    if(!checker){

        printf("FAILED: entry_list_hasInIt(head,fb) expected fb\n");
        return 1;
    }


    return 0;
}


int main(void) {
    int failures = 0;

    failures += single();
    failures += array();
    failures += mapping();
    failures += record();
    failures += undefined();

    failures += wrappers();
    failures += nameCopy();

    failures += linkedListTest();

    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("TEST FAILURES: %d\n", failures);
        return 1;
    }
}

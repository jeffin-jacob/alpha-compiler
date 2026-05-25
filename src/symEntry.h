#pragma once
#include <stdlib.h>
#include <stdbool.h>
typedef struct symbolTable symbolTable; //forward declaration for the symbol table, used for records
typedef struct symEntry symEntry;  
typedef enum {
  ANN_TYPE,
  ANN_FUNCTION,
  ANN_LOCAL,
  ANN_PARAMETER
} annotationKind;

//size suffixes also in backend
//but need here too for symEntry to access it i think
#define B 2
#define L 1
#define Q 0

typedef enum {TYPE_PRIMITIVE, TYPE_SINGLE, TYPE_MAPPING, TYPE_RECORD, TYPE_ARRAY, TYPE_UNDEFINED } typeKind; //for the types

typedef enum {PRIM_BOOL, PRIM_CHAR, PRIM_INT, PRIM_ADDR} primitives; //for primitive types to be able to calc offsets

typedef struct typeExpr {
  typeKind kind;
  union {
    symEntry *single; //used for single type, can point to primitives or user defined types
    struct { symEntry *dom, *rng; } map; //mapping, points to the domain and range symbol table entries
    struct {int entryCount; symbolTable *scope;} rec ;   // pointer to the symbol table scope for records
    struct { int dim; symEntry *elem; } arr; //for arrays, number of dimensions, and the type of the entries in the array
    primitives prim; //for primitive types to calc offsets
  } as;
} typeExpr;

//TODO: Add a variable that stores offset for local variables/params/rec entries
// and a function start position in IR array kind of thing
typedef struct symEntry {
  //Everything here is used in parsing/typechecking
  char *name; 
  typeExpr *type; 
  annotationKind ann;  // type/function/local/parameter
  int offset;          // offset should be negative for scope locals, positive for rec locals
                       // positive w/ room for bookkeeping in params
                       // for functions we can store the into this 'offset' var
  struct symEntry *paramOf;       // only for ANN_PARAMETER, else NULL
  struct symEntry *next; //next entry in the linkedList
  bool isAs; //if this is an entry for a function, 
  bool isFuncPtr; // for mapping symEntries, that are not directly functions
  //Everything below here is used for compiler backend stuff (register allocation etc)
  bool not_in_memory; //this is set to true when this variables value is modified while in a register so we know to write back to memory if necessary
  int reg_num; //Which register descriptor entry this is a part of if any, -1 means not in register
  bool alive;  //Liveness value
  int next_use;//Which instruction in ir array is it next used, -1 means outside of current block

} symEntry;

// typeExpr constructors 
typeExpr *type_primitive(primitives p);                 // TYPE_PRINGLE
typeExpr *type_single(symEntry *typeSym);                 // TYPE_SINGLE
typeExpr *type_array(symEntry *typeSym,int numDimensions);                 // TYPE_ARRAY
typeExpr *type_mapping(symEntry *dom, symEntry *rng);     // TYPE_MAPPING
typeExpr *type_record( int entryCount,symbolTable *scope);               // TYPE_RECORD (just stores pointer)
typeExpr *type_undefined(void);                           // TYPE_UNDEFINED


//  symEntry constructors 
symEntry *sym_make(const char *newName, annotationKind newAnn, typeExpr *newType, symEntry *newParamOf); 
// newParamOf is NULL if no ANN_PARAMETER

// specific symEntry constructors
symEntry *sym_make_type(const char *name, typeExpr *type);
symEntry *sym_make_function(const char *name, typeExpr *type);
symEntry *sym_make_local(const char *name, typeExpr *type);
symEntry *sym_make_param(const char *name, typeExpr* type, symEntry *fnEntry);


symEntry* function_get_domain(symEntry* fnEntry);
symEntry* function_get_range(symEntry* fnEntry);

// offset stack for local variables
typedef struct scope_stack scope_stack;
struct scope_stack {
    int cur_offset;
    scope_stack* prev;
};
extern scope_stack* ScopeStack;
extern int local_offset;

void sym_set_offset(symEntry* e);
void update_offset_local(symEntry* e);
void scope_push();
void scope_pop();

// symEntry list functions
void entry_list_append(symEntry *head, symEntry *node);

// search for name within a single scope
symEntry *entry_list_find(symEntry *head, const char *name);

bool entry_list_hasInIt(symEntry *head, symEntry *entry);

bool entry_list_has_param_of(symEntry *head, symEntry *func);
//for debugging
int entry_list_length(symEntry *head);

typeExpr *get_type(symEntry *entry);

size_t get_size(symEntry *entry);
int get_size_ss(symEntry *entry);

#include "symEntry.h"
#include "symbol-table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern bool debugflag;

typeExpr *type_primitive(primitives p){ // TYPE_PRIMITIVE
	typeExpr *retVal =  (typeExpr *)calloc(1, sizeof(typeExpr));
	if (!retVal) return NULL;
	retVal -> kind = TYPE_PRIMITIVE;
    retVal->as.prim = p;
	return retVal;
}      
typeExpr *type_single(symEntry *typeSym){ // TYPE_SINGLE
	if (!typeSym) return NULL;
	typeExpr *retVal =  (typeExpr *)calloc(1, sizeof(typeExpr));
	if (!retVal) return NULL;
	retVal -> kind = TYPE_SINGLE;
	retVal -> as.single = typeSym;
	return retVal;
}
typeExpr *type_array(symEntry *typeSym, int numDimensions){ // TYPE_SINGLE
        if (!typeSym) return NULL;
	if (!numDimensions) return NULL;
	if(numDimensions < 1) return NULL;
        typeExpr *retVal =  (typeExpr *)calloc(1, sizeof(typeExpr));
        if (!retVal) return NULL;
        retVal -> kind = TYPE_ARRAY;
        retVal -> as.arr.elem = typeSym;
	retVal -> as.arr.dim = numDimensions;
        return retVal;
}
typeExpr *type_mapping(symEntry *dom, symEntry *rng){     // TYPE_MAPPING
  if (!dom || !rng){
    if (debugflag) { printf("mapping not found \n");} 
    return NULL;
  }
	
	typeExpr *retVal =  (typeExpr *)calloc(1, sizeof(typeExpr));
	if (!retVal) return NULL;
	retVal -> kind = TYPE_MAPPING;
	retVal -> as.map.dom = dom;
	retVal -> as.map.rng = rng;
	//printf("retVal mapping: %p \n", retVal);
	return retVal;
}        
typeExpr *type_record(int entryCount, symbolTable *scope){               // TYPE_RECORD (just stores pointer)
	//if (!scope) return NULL;
        symEntry *entry = scope -> entry;
	while(entry != NULL){
	  int curOffset = entry -> offset;
	  int entrySize = get_size(entry);
	  //printf("curOffset: %d \n", curOffset);
	  //printf("entrySize: %d \n", entrySize);
	  entry -> offset = (curOffset + entrySize) * -1;
	  entry = entry -> next;
	}
	typeExpr *retVal =  (typeExpr *)calloc(1, sizeof(typeExpr));
	if (!retVal) return NULL;
	retVal -> kind = TYPE_RECORD;
	retVal -> as.rec.scope = scope;
	retVal -> as.rec.entryCount = entryCount;
	
	return retVal;

}
typeExpr *type_undefined(void){                           // TYPE_UNDEFINED
	typeExpr *retVal =  (typeExpr *)calloc(1, sizeof(typeExpr));
	if (!retVal) return NULL;
	retVal -> kind = TYPE_UNDEFINED;
	return retVal;

}

symEntry *sym_make(const char *newName, annotationKind newAnn, typeExpr *newType, symEntry *newParamOf){
	if(!newName || !newType){ return NULL;}
	symEntry *retVal =  (symEntry *)calloc(1, sizeof(symEntry));
	if (!retVal) return NULL;
	retVal->name = malloc(strlen(newName) + 1);
	strcpy(retVal -> name, newName);
	retVal -> type = newType;
	retVal->ann = newAnn;
	if (newAnn == ANN_PARAMETER && !newParamOf) return NULL;
	if (newAnn != ANN_PARAMETER && newParamOf) return NULL;

	retVal->paramOf = newParamOf;
	//retVal -> isFuncPtr = false;
	if(newType -> kind == TYPE_MAPPING){
	  retVal -> isFuncPtr = true;
	  //printf("setting isFuncPtr true for: %s \n", newName);
	}
    if (newAnn == ANN_LOCAL) { //if a local var then set offset
        update_offset_local(retVal);
        sym_set_offset(retVal);
    }
    //else if (newAnn == ANN_REC) { //if a local var then set offset
    //    update_offset_local(retVal);
    //    sym_set_offset(retVal);
    //}
	return retVal;


}; 
symEntry *sym_make_type(const char *name, typeExpr *type){
	return sym_make(name, ANN_TYPE, type, NULL);


}
symEntry *sym_make_function(const char *name, typeExpr *type){
       symEntry *retVal = sym_make(name, ANN_FUNCTION, type, NULL);
       retVal -> isFuncPtr = false;
       return retVal;

}
symEntry *sym_make_local(const char *name, typeExpr *type){
     symEntry *retVal =  sym_make(name, ANN_LOCAL, type, NULL);
     if(type -> kind == TYPE_MAPPING){
       retVal -> isFuncPtr = true;
     }
     return retVal;

}

void sym_set_offset(symEntry* e) {
    e->offset = local_offset;
}

// offset stack for local variables
scope_stack* ScopeStack;
int local_offset;

void scope_push() {
    scope_stack* s = malloc(sizeof(scope_stack));
    s->cur_offset = local_offset;
    s->prev = ScopeStack;
    ScopeStack = s;
}

void scope_pop() {
    local_offset = ScopeStack->cur_offset;
    ScopeStack = ScopeStack->prev;
}

void update_offset_local(symEntry* e) {
    if (e->type->kind == TYPE_SINGLE && e->type->as.single->type->kind==TYPE_PRIMITIVE) { //if primitive then update offset appropriately
        switch (e->type->as.single->type->as.prim) {
        case PRIM_BOOL:
            local_offset-=1;
            break;
        case PRIM_CHAR:
            local_offset-=1;
            break;
        case PRIM_INT:
            local_offset-=4;
            local_offset &= ~3;
            break;
        case PRIM_ADDR:
            local_offset-=8;
            local_offset &= ~7;
            break;
        }
    } else { //if not primitive its always 8 bytes
        local_offset-=8;
        local_offset &= ~7;
    }
}

symEntry* function_get_domain(symEntry* fnEntry) {
    //Say fnEntry is type T1 and T1 is integer->integer
    //fnEntry->type->as.single->type->as.map.dom
    //         (T1)              ^this type is integer->integer
  //printf("function name: %s \n", fnEntry-> name);
  typeExpr *fnType = fnEntry->type;
  //printf("we got fnType \n");
  symEntry *single = (fnType -> as).single;
  //printf("we got the single: %p \n", single);
  typeExpr *singleType = single -> type;
  //printf("got single type\n");
  typeKind kind = singleType -> kind;
  //printf("we got the kind %d\n",kind);
  switch(kind){
  case TYPE_PRIMITIVE:
    //printf("primitive \n");
    break;
  case TYPE_SINGLE:
    //printf("single \n");
    break;
  case TYPE_MAPPING:
    //printf("mapping \n");
    break;
  case TYPE_RECORD:
    //printf("record \n");
    break;
  case TYPE_ARRAY:
    //printf("array \n");
    break;
  case TYPE_UNDEFINED:
    //printf("undefined \n");
    break;
  }
  //printf("dom: %p \n", (singleType ->as).map.dom);
    return singleType->as.map.dom;
}

symEntry* function_get_range(symEntry* fnEntry) {
    //Say fnEntry is type T1 and T1 is integer->integer
    //fnEntry->type->as.single->type->as.map.dom
    //         (T1)              ^this type is integer->integer
    return fnEntry->type->as.single->type->as.map.rng;
}

symEntry *sym_make_param(const char *name, typeExpr* type, symEntry *fnEntry){
	return sym_make(name, ANN_PARAMETER, type, fnEntry);
}

void entry_list_append(symEntry *head, symEntry *node){
	if(head != NULL){
		symEntry *cur = head;
		while(cur -> next != NULL){
			cur = cur -> next;
		
		}
		cur -> next = node;
		
	
	}
}

symEntry *entry_list_find(symEntry *head, const char *name) {
    if (!head || !name) return NULL;

    for (symEntry *cur = head; cur != NULL; cur = cur->next) {
        if (cur->name && strcmp(cur->name, name) == 0) {
            return cur;
        }
    }
    return NULL;
}

bool compareEntries(symEntry *e1, symEntry *e2){
  if(!e1 || !e2) return false;
  if(!(e1 -> name) || !(e2 -> name)) return false;
  if( strcmp(e1->name, e2->name) != 0) return false;
  if( e1 -> type != e2 -> type) return false;
  if(e1 -> ann != e2 -> ann) return false;
  if(e1 -> ann == ANN_PARAMETER) {
    if(e1 -> paramOf != e2 -> paramOf) return false;

  }
  return true;


}
bool entry_list_hasInIt(symEntry *head, symEntry *entry){
  symEntry *cur = head;
  bool retVal = false;
  while(cur && !retVal){
    if(compareEntries(cur,entry)){
	retVal = true;

      }
    else{
      cur = cur -> next;
    }

  }
    return retVal;
}

bool entry_list_has_param_of(symEntry *head, symEntry *func) {
    symEntry *curr_node = head;
    while (curr_node != NULL) {
        if (curr_node->paramOf == func) {
            return true;
        }
        curr_node = curr_node->next;
    }
    return false;
}

int entry_list_length(symEntry *head) {
    int counter = 0;
    for (symEntry *cur = head; cur != NULL; cur = cur->next) {
        counter++;
    }
    return counter;
}
typeExpr *get_type(symEntry *entry) {
    if (entry == NULL || entry->type == NULL) {
        return NULL;
    }

    typeExpr *curType = entry->type;

    while (curType != NULL && curType->kind == TYPE_SINGLE) {
        symEntry *next = curType->as.single;
        if (next == NULL || next->type == NULL) {
            break;
        }
        curType = next->type;
    }
    return curType;
}
//return size in bytes that entry itself occupies in memory
int get_size_ss(symEntry *entry) {
  //typeExpr *curType = get_type(entry);
  //printf("trying to get register sizing for: %s \n", entry -> name);
    int a = get_size(entry);
    if ( a == 1 ) return B; //boolean/character
    if ( a == 4 ) return L; //integer
    if ( a == 8 ) return Q; //other
    return -1;
}

/* Returns the size (in bytes) that an entries type <entry> occupies in memory */
size_t get_size(symEntry *entry) {
    size_t ret;
    typeExpr *type = get_type(entry); //entry->type;
    if (type->kind == TYPE_PRIMITIVE) {
        const primitives prim_type = type->as.prim;
        switch (prim_type) {
            case PRIM_BOOL:
                ret = 1;
                break;
            case PRIM_CHAR:
                ret = 1;
                break;
            case PRIM_INT:
                ret = 4;
                break;
            case PRIM_ADDR:
                ret = 8;
                break;
        }
    } else {
        ret = 8;
    }
    return ret;
}

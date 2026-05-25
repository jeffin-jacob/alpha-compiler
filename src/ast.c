
#include "ast.h"
#include "symEntry.h"
#include "../src/ast.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

extern bool debugflag;

int parseArrayInt(const char *s) {
    if (s == NULL || s[0] != '_') return -1;

    int result = 0;

    // start after '_'
    for (int i = 1; s[i] != '\0'; i++) {
        if (!isdigit((unsigned char)s[i])) {
            return -1;  // invalid character
        }

        result = result * 10 + (s[i] - '0');
    }

    // edge case: "_" only → invalid
    if (s[1] == '\0') return -1;

    return result;
}

bool is_dot_op_type_safe(Assignable *parent,
                         Assignable *LHChild,
                         Basic *RHChild) {
    typeExpr *LHChild_type = LHChild->type_info;
    if (LHChild_type->kind == TYPE_SINGLE) {
        symEntry *type_entry = LHChild_type->as.single;
        typeExpr *LHChild_type_of_type =  get_type(type_entry);
        if (LHChild_type_of_type->kind == TYPE_RECORD) {
            symbolTable *record_scope = LHChild_type_of_type->as.rec.scope;
            symEntry *entry = getEntryInSymbolTable(record_scope, RHChild->name, false); 
            if (entry != NULL) {
                parent->type_info = get_type(entry);
		parent -> resolved_type = entry;
                parent->type = RECORD_ACCESS;
            if (debugflag) {
		        printf("determined that this is a record_access \n");
            }
		return true;
            } 
        } 
    }
    else if (LHChild_type->kind == TYPE_RECORD) {
            symbolTable *record_scope = LHChild_type->as.rec.scope;
            symEntry *entry =
                getEntryInSymbolTable(record_scope, RHChild->name, false);
            if (entry != NULL) {
                parent->type_info = get_type(entry);
                parent -> resolved_type = entry;
                parent->type = RECORD_ACCESS;
            if (debugflag) {
		        printf("determined that this is a record_access \n");
            }
                return true;
            }
        }
    else if (LHChild_type->kind == TYPE_ARRAY) {
        char *name = RHChild -> name;
	int getNum = parseArrayInt(name);
	if(getNum != -1){
	  
	  symEntry *intEntry = getEntryInSymbolTable(getSymbolTable(), "integer", true);
	  parent->type_info = get_type(intEntry);
	  parent -> resolved_type = intEntry;
          parent->type = ARRAY_DIM_LOOKUP;
        
	  return true;
	}
        
    }
    if (debugflag) {
        printf("got zilch, nada, nothing \n");
    }
    parent->type_info = type_undefined();
    return false;
}

bool is_ablock_type_safe(Assignable *parent, Assignable *id, ExprList *block) {
    typeExpr *id_type = id->type_info;
    if (id_type->kind == TYPE_SINGLE) {
        symEntry *type_entry = id_type->as.single;
        typeExpr *id_type_of_type = get_type(type_entry);
        if (id_type_of_type->kind == TYPE_MAPPING) {

            /* TODO type check ablock */

            symEntry *function_range = id_type_of_type->as.map.rng;
            if (function_range != NULL) {
                parent->type_info = type_single(function_range);
                parent -> resolved_type = function_range;		
		return true;
            } 
        } 
    }
    else if (id_type->kind == TYPE_MAPPING) {

            /* TODO type check ablock */

            symEntry *function_range = id_type->as.map.rng;
            if (function_range != NULL) {
                parent->type_info = type_single(function_range);
                parent -> resolved_type = function_range;
                return true;
            }
        }
    else if (id_type->kind == TYPE_ARRAY) {

        /* TODO type check ablock */

        parent->type_info = id_type;
	parent -> resolved_type = id -> resolved_type;
	return true;
    }
    parent->type_info = type_undefined();
    return false;
}

bool is_mem_op_type_safe(Expr *parent,
                         Assignable *child,
                         symEntry *parent_target) {
    typeExpr *child_type = child->type_info;
    if (child_type->kind == TYPE_SINGLE) {
        symEntry *type_entry = child_type->as.single;
        typeExpr *child_type_of_type = type_entry->type;
        if (child_type_of_type->kind == TYPE_RECORD) {
            parent->type_info = type_single(parent_target);
            return true;
        } 
    }
    if(child_type->kind == TYPE_RECORD){
            parent->type_info = type_single(parent_target);
            return true;
    }
    else if (child_type->kind == TYPE_ARRAY) {
        parent->type_info = type_single(parent_target);
        return true;
    }
    parent->type_info = type_undefined();
    return false;
}

bool is_while_stmt_type_safe(Expr *cond, symEntry *cond_target) {
  if (debugflag) {
  printf("typechecking while \n");
  }
    typeExpr *cond_type = cond->type_info;
    //printf("cond_type: %p \n", cond_type);
    if (cond_type->kind == TYPE_SINGLE) {
      //printf("cond_type -> kind  %d\n", cond_type -> kind);
      typeExpr *condUnwrapped = get_type(cond_type->as.single);
      if (debugflag) {
        printf("condUnwrapped: %p \n", condUnwrapped);
        printf("boolean type: %p \n", cond_target -> type);
      }
      //symEntry *type_entry = cond_type->as.single;
        if (condUnwrapped == cond_target -> type) {
            return true;
        }
    }
    else if(cond_type -> kind == TYPE_PRIMITIVE){
      if (cond_type == cond_target -> type) {
            return true;
        }
      
    }
    return false;
}

static void asc_error(int line, int col, const char *msg, FILE *asc_file) {
    if (asc_file)
        fprintf(asc_file, "LINE %03d:%d ** ERROR: %s\n", line, col, msg);
    fprintf(stderr, "LINE %03d:%d ** ERROR: %s\n", line, col, msg);
}
AstExpr *ast_new_reserve(Assignable *assignable, symEntry *addressEntry){
  AstExpr *retVal = (AstExpr *) calloc(1, sizeof(AstExpr));
  retVal -> kind = AST_EXPR_MEMOP;
  retVal -> line = assignable -> line;
  retVal -> col = assignable -> col;
  retVal -> data.assignable = &(assignable -> basic);
  retVal -> resolved_type = addressEntry;
  return retVal;
}
AstExpr *ast_new_const_expr(ConstantValue *constant){
  if (!constant) return NULL;
  AstExpr *retVal = (AstExpr *) calloc(1, sizeof(AstExpr));
  retVal -> kind = AST_EXPR_CONST;
  retVal -> line = constant -> line;
  retVal -> col = constant -> col;
  retVal -> data.constant = constant;
  return retVal;
}

AstExpr *ast_new_assignable_expr(Assignable *assignable){
   if (!assignable) return NULL;
   AstExpr *retVal = (AstExpr *) calloc(1, sizeof(AstExpr));

   retVal -> kind = AST_EXPR_ASSIGNABLE;

   retVal -> line =  assignable -> line;

   retVal -> col = assignable -> col;
   retVal -> data.assignable = &(assignable -> basic);
   retVal -> resolved_type = assignable -> resolved_type;
   //printf("resolved_type of assingable astExpr: %p \n", retVal -> resolved_type);
   return retVal;
}
AstExpr *ast_new_unary_expr(AstExprKind kind, AstExpr *child, int line, int col){
 
   if (!child) return NULL;  
   AstExpr *retVal = (AstExpr *) calloc(1, sizeof(AstExpr));
   retVal -> kind = kind;
   retVal -> line = line;
   retVal -> col =  col;
   retVal -> data.unary.child = child;
   return retVal;
   
}
AstExpr *ast_new_binary_expr(AstExprKind kind, AstExpr *left, AstExpr *right, int line, int col){
    if (!left) return NULL;
   if (!right) return NULL;
   AstExpr *retVal = (AstExpr *) calloc(1, sizeof(AstExpr));
   retVal -> kind = kind;
   retVal -> line = left -> line;
   retVal -> col = left -> col;
   retVal -> data.binary.left = left;
   retVal -> data.binary.right = right;
    return retVal;

}

AstStmt *ast_new_assign_stmt(struct Assignable *target, AstExpr *value, int line, int col){
   if(!target) return NULL;
  if(!value) return NULL;
  AstStmt *retVal = (AstStmt *) calloc(1, sizeof(AstStmt));
  retVal -> kind = AST_STMT_ASSIGN;
  retVal -> line =  line;
  retVal -> col = col;
  retVal -> data.assign_stmt.target = target;
  retVal -> data.assign_stmt.value = value;
  //  retVal -> resolved_type = target -> resolved_type;
   return retVal;

}
AstStmt *ast_new_return_stmt(AstExpr *target, symEntry *range, int line, int col){
   if(!target) return NULL;
  if(!range) return NULL;
  AstStmt *retVal = (AstStmt *) calloc(1, sizeof(AstStmt));
  retVal -> kind = AST_STMT_RETURN;
  retVal -> line = line;
  retVal -> col =  col;
  retVal -> data.return_stmt.target = target;
  retVal -> data.return_stmt.range = range;
  return retVal;

}
AstStmt *ast_new_if_stmt(AstExpr *condition, AstStmt *then_block, AstStmt *else_block, int line, int col){
   if (!condition) return NULL;
  if (!then_block) return NULL;
  if (!else_block) return NULL;
  AstStmt *retVal = (AstStmt *) calloc(1, sizeof(AstStmt));
  retVal -> kind = AST_STMT_IF;
  retVal -> line = condition -> line;
  retVal -> col = condition -> col;
  retVal -> data.if_stmt.condition = condition;
  retVal -> data.if_stmt.then_block = then_block->data.block_stmt.statements;
  retVal -> data.if_stmt.else_block = else_block->data.block_stmt.statements;
   return retVal;
}

AstStmt *ast_new_block_stmt(AstStmtList *statements, symbolTable *scope, int line, int col) {
     if (!statements) return NULL;
    AstStmt *retVal = calloc(1, sizeof(AstStmt));
    retVal->kind = AST_STMT_BLOCK;
    retVal->line = line;
    retVal->col = col;
    retVal->data.block_stmt.statements = statements;
    retVal->data.block_stmt.scope = scope;
     return retVal;
}

AstStmtList *ast_new_stmt_list(AstStmt *stmt){
   if (!stmt) return NULL;
  AstStmtList *retVal = (AstStmtList *) calloc(1, sizeof(AstStmtList));
  
  retVal -> stmt = stmt;
   return retVal;

}
AstStmtList *ast_stmt_list_append(AstStmtList *list, AstStmt *stmt){
   if (!list) return NULL;
  if (!stmt) return NULL;
  AstStmtList *cursor = list;
  while(cursor -> next){
    cursor = cursor -> next;

  }
  AstStmtList *newVal = (AstStmtList *) calloc(1, sizeof(AstStmtList));
  newVal -> stmt = stmt;
  cursor -> next = newVal;
   return list;
}
void typecheck_stmt_list(AstStmtList *list, FILE *asc,symbolTable *table,bool type_checker) {
   while (list) {
     typecheck_stmt(list->stmt, asc,table,type_checker);
    list = list->next;
  }
   }

bool typecheck_stmt(AstStmt *stmt, FILE *asc, symbolTable *table, bool type_checker) 
/* Call to "get_type" would cause segfault when record or array is on LHS of 
 * assignment */
/* { */
/*     return true; */
/* } */
{
   if (!stmt) return false;
    
    switch (stmt->kind) {
    case AST_STMT_ASSIGN: {
      //printf("we are the assign check\n");
      Assignable *target = stmt->data.assign_stmt.target;
      //printf("we have the basic: %p \n", target);
      if (!target) return false;

      if(!(target -> resolved_type)){
	if(type_checker){asc_error(target->line, target->col, "assignment target has invalid type", asc);}
            return false;
      
      }
        symEntry *findLType =target->resolved_type-> type->as.single;
	//printf("we have the findLType: %p \n", findLType);
        if (!findLType) {
	  if(type_checker){asc_error(target->line, target->col, "assignment target has invalid type", asc);}
            return false;
        }

        AstExpr *getRhs = stmt->data.assign_stmt.value;
	//printf("we have the getRhs: %p \n", getRhs);
        symEntry *findRType = typecheck_expr(getRhs, asc, table,type_checker);
        //printf("we have the findRType: %p \n", findRType);
	if (!findRType) return false;
	typeKind lKind = get_type(findLType) -> kind;
	bool addressCheck = (lKind == TYPE_RECORD) || (lKind == TYPE_ARRAY);
	if (addressCheck){
      if (debugflag) {
	    printf("address check went through \n");
      }
	     symEntry *addressEntry = getEntryInSymbolTable(table, "address", true);
	     if(get_type(findRType) == get_type(addressEntry)){
	       return true;
	     }
	     //printf("findRType typeExpr: %p \n", get_type(findRType));
	     //printf("addressEntry typeExpr: %p \n", get_type(addressEntry));
	}
        if (get_type(findLType) != get_type(findRType)) {
	  if(type_checker){asc_error(target->line, target->col, "assignment type mismatch", asc);}
	    return false;
        if (debugflag) {
	      printf("findRLType typeExpr: %p \n", get_type(findLType));
              printf("findRType typeExpr: %p \n", get_type(findRType));
        }
	}

        break;
    }
    case AST_STMT_RETURN: {
        AstExpr *target = stmt->data.return_stmt.target;
        if (!target) return false;
	
	//printf("type checking the expression in the return\n");
        symEntry *findLType = typecheck_expr(target,asc,table,type_checker);
	//printf("got through the type check, findLType: %p\n", findLType);
        //printf("lType name %s \n", findLType -> name);
	if (!findLType) {
	  if(type_checker){asc_error(target->line, target->col, "returning expression has no type", asc);}
            return false;
        }
        symEntry *findRType = stmt->data.return_stmt.range;
        //printf("this is the rType: %p \n",findRType);
	//printf("rType name %s \n", findRType -> name);
        if (!findRType) return false;
        if (get_type(findLType) != get_type(findRType)) {
	  if(type_checker){asc_error(target->line, target->col, "return type mismatch", asc);}
	    return false;
	}
	break;
    }
    case AST_STMT_IF: {
         AstExpr *condition = stmt->data.if_stmt.condition;
        if (!condition) return false;

        AstStmtList *then_block = stmt->data.if_stmt.then_block;
        AstStmtList *else_block = stmt->data.if_stmt.else_block;
        if (!then_block || !else_block) return false;

        symEntry *getCondType = typecheck_expr(condition, asc, table,type_checker);
        symEntry *getBoolType = getEntryInSymbolTable(table, "Boolean", true);

        if (get_type(getCondType) != get_type(getBoolType)) {
	  
          if(type_checker){  asc_error(condition->line, condition->col, "if condition must be Boolean", asc);}
	    return false;
	}

	//        typecheck_stmt_list(then_block, asc, table);
        //typecheck_stmt_list(else_block, asc, table);
        break;
    }

    case AST_STMT_BLOCK:
      typecheck_stmt_list(stmt->data.block_stmt.statements, asc, stmt->data.block_stmt.scope,type_checker);
        	break;
    }
    return true;
}

symEntry *typecheck_expr(AstExpr *expr, FILE *asc, symbolTable *table, bool type_checker) {
   if (!expr) return NULL;

    switch (expr->kind) {
    case AST_EXPR_CONST: {
        ConstantValue *constant = expr->data.constant;
        if (!constant) return NULL;

        switch (constant->type) {
        case ADDRESS:
            expr->resolved_type = getEntryInSymbolTable(table, "address", true);
            return expr->resolved_type;
        case INTEGER:
            expr->resolved_type = getEntryInSymbolTable(table, "integer", true);
	    //printf("got integer type for constant: %p \n", expr->resolved_type);
	    return expr->resolved_type;
        case BOOLEAN:
            expr->resolved_type = getEntryInSymbolTable(table, "Boolean", true);
            return expr->resolved_type;
        case CHARACTER:
            expr->resolved_type = getEntryInSymbolTable(table, "character", true);
            return expr->resolved_type;
        case STRING:
            expr->resolved_type = getEntryInSymbolTable(table, "string", true);
            return expr->resolved_type;
        case NULLTYPE:
            expr->resolved_type = getEntryInSymbolTable(table, "null", true);
            return expr->resolved_type;
        default:
	  if(type_checker) {asc_error(expr->line, expr->col, "unknown constant type", asc);}
            return NULL;
        }
	 }

    case AST_EXPR_ASSIGNABLE: {
	 return expr->resolved_type;
    }
    case AST_EXPR_MEMOP: {
      return expr -> resolved_type;
    }
    case AST_EXPR_SUB:
    case AST_EXPR_MUL:
    case AST_EXPR_DIV:
    case AST_EXPR_REM:
    case AST_EXPR_ADD: {
         AstExpr *left = expr->data.binary.left;
        AstExpr *right = expr->data.binary.right;
        if (!left || !right) return NULL;

        symEntry *getLType = typecheck_expr(left, asc, table,type_checker);
        symEntry *getRType = typecheck_expr(right, asc, table,type_checker);
        symEntry *getIntType = getEntryInSymbolTable(table, "integer", true);
        if ( get_type(getLType) == get_type(getIntType) && get_type(getRType) == get_type(getIntType)) {
            expr->resolved_type = getIntType;
            return getIntType;
        }
	//printf("lType: %s \n", getLType -> name);
	//printf("lType: %p \n", get_type(getLType));
	typeKind lKind = get_type(getLType) -> kind;
	//char *lTypeName = NULL;
	switch(lKind){
	case TYPE_PRIMITIVE:
	  //lTypeName = "PRIMITIVE";
	  break;
	case TYPE_SINGLE:
          //lTypeName = "SINGLE";
          break;
        case TYPE_MAPPING:
          //lTypeName = "MAPPING";
          break;
        case TYPE_RECORD:
          //lTypeName = "RECORD";
          break;
        case TYPE_ARRAY:
          //lTypeName = "ARRAY";
          break;
        case TYPE_UNDEFINED:
          //lTypeName = "UNDEFINED";
          break;	  
	}
	//printf("lTypeKind: %s \n", lTypeName);
	//printf("rType: %s \n",getRType -> name);
	//printf("rType: %p \n", get_type(getRType));
        typeKind rKind = get_type(getLType) -> kind;
        //char *rTypeName = NULL;
        switch(rKind){
        case TYPE_PRIMITIVE:
          //rTypeName = "PRIMITIVE";
          break;
        case TYPE_SINGLE:
          //rTypeName = "SINGLE";
          break;
        case TYPE_MAPPING:
          //rTypeName = "MAPPING";
          break;
        case TYPE_RECORD:
          //rTypeName = "RECORD";
          break;
        case TYPE_ARRAY:
          //rTypeName = "ARRAY";
          break;
        case TYPE_UNDEFINED:
	  //  rTypeName = "UNDEFINED";
          break;
        }
        //printf("rTypeKind: %s \n", rTypeName);
	
        //printf("int: %s \n",getIntType -> name);
        //printf("int: %p \n", get_type(getIntType));
        //typeKind intKind = get_type(getIntType) -> kind;
        //char *intTypeName = NULL;
        switch(lKind){
        case TYPE_PRIMITIVE:
          //intTypeName = "PRIMITIVE";
          break;
        case TYPE_SINGLE:
          //intTypeName = "SINGLE";
          break;
        case TYPE_MAPPING:
          //intTypeName = "MAPPING";
          break;
        case TYPE_RECORD:
	  // intTypeName = "RECORD";
          break;
        case TYPE_ARRAY:
          //intTypeName = "ARRAY";
          break;
        case TYPE_UNDEFINED:
          //intTypeName = "UNDEFINED";
          break;
        }
        //printf("lTypeKind: %s \n", intTypeName);
            expr->resolved_type = getIntType;
	    if(type_checker){
	      asc_error(expr->line, expr->col, "arithmetic operands must both be integer", asc); }
        return getIntType;
    }

    case AST_EXPR_AND:
    case AST_EXPR_OR: {
          AstExpr *left = expr->data.binary.left;
        AstExpr *right = expr->data.binary.right;
        if (!left || !right) return NULL;

        symEntry *getLType = typecheck_expr(left, asc, table,type_checker);
        symEntry *getRType = typecheck_expr(right, asc, table,type_checker);
        symEntry *getBoolType = getEntryInSymbolTable(table, "Boolean", true);

        if (get_type( getLType) == get_type(getBoolType) && get_type( getRType) == get_type(getBoolType)) {
            expr->resolved_type = getBoolType;
            return getBoolType;
        }
        expr->resolved_type = getBoolType;
	if(type_checker){
	  asc_error(expr->line, expr->col, "boolean operands required for and/or", asc);}
        return getBoolType;
    }

    case AST_EXPR_NOT: {
        AstExpr *child = expr->data.unary.child;
        if (!child) return NULL;

        symEntry *getChildType = typecheck_expr(child, asc, table,type_checker);
        symEntry *getBoolType = getEntryInSymbolTable(table, "Boolean", true);

        if (get_type(getChildType) == get_type( getBoolType)) {
            expr->resolved_type = getBoolType;
            return getBoolType;
        }
        expr->resolved_type = getBoolType;
	if(type_checker){
	  asc_error(expr->line, expr->col, "not operand must be Boolean", asc); }
        return getBoolType;
    }

    case AST_EXPR_NEG: {
         AstExpr *child = expr->data.unary.child;
        if (!child) return NULL;

        symEntry *getChildType = typecheck_expr(child, asc, table,type_checker);
        symEntry *getIntType = getEntryInSymbolTable(table, "integer", true);

        if (get_type(getChildType) == get_type(getIntType)) {
            expr->resolved_type = getIntType;
	     return getIntType;
        }
            expr->resolved_type = getIntType;
	    if(type_checker){
	      asc_error(expr->line, expr->col, "unary minus operand must be integer", asc);}
        return getIntType;
    }

    case AST_EXPR_LT: {
        AstExpr *left = expr->data.binary.left;
        AstExpr *right = expr->data.binary.right;
        if (!left || !right) return NULL;

        symEntry *getLType = typecheck_expr(left, asc, table,type_checker);
        symEntry *getRType = typecheck_expr(right, asc, table,type_checker);
        symEntry *getBoolType = getEntryInSymbolTable(table, "Boolean", true);
        symEntry *getIntType = getEntryInSymbolTable(table, "integer", true);
        symEntry *getCharType = getEntryInSymbolTable(table, "character", true);

        bool getLCheck = get_type( getLType) == get_type( getBoolType) || get_type( getLType) == get_type(getIntType) || get_type(getLType) == get_type(getCharType);
	bool getRCheck = get_type(getRType) == get_type(getBoolType) || get_type(getRType) == get_type(getIntType) || get_type(getRType) == get_type(getCharType);

        if (!getLCheck) {
            expr->resolved_type = getBoolType;
	    if(type_checker){
	      asc_error(left->line, left->col, "left operand of < has invalid type", asc);}
            return getBoolType;
        }
        if (!getRCheck) {
	              expr->resolved_type = getBoolType;
		      if(type_checker){
			asc_error(right->line, right->col, "right operand of < has invalid type", asc);}
            return getBoolType;
        }
        if (get_type(getLType) == get_type(getRType)) {
            expr->resolved_type = getBoolType;
            return getBoolType;
        }
            expr->resolved_type = getBoolType;
	    if(type_checker){
	      asc_error(expr->line, expr->col, "operands of < must have the same type", asc);}
        return getBoolType;
    }

    case AST_EXPR_EQ: {
        AstExpr *left = expr->data.binary.left;
        AstExpr *right = expr->data.binary.right;
        if (!left || !right) return NULL;

        symEntry *getLType = typecheck_expr(left, asc, table,type_checker);
        symEntry *getRType = typecheck_expr(right, asc, table,type_checker);
        symEntry *getBoolType = getEntryInSymbolTable(table, "Boolean", true);
        symEntry *getNullType = getEntryInSymbolTable(table, "null", true);

        if (!getLType || !getRType) return NULL;

        bool lNullCheck = (get_type(getLType) == get_type(getNullType));
        bool rNullCheck = (get_type(getRType) == get_type(getNullType));

        if (lNullCheck && rNullCheck) {
                expr->resolved_type = getBoolType;
		if(type_checker){
		  asc_error(expr->line, expr->col, "cannot compare null with null", asc);}
	  return getBoolType;
        }

        if (lNullCheck && !rNullCheck) {
            typeKind curKind = getRType->type -> kind;
            bool recArrFun = (curKind == TYPE_MAPPING || curKind == TYPE_ARRAY || curKind == TYPE_RECORD);
            if (recArrFun) {
                expr->resolved_type = getBoolType;
                return getBoolType;
            } else {
	                      expr->resolved_type = getBoolType;
			      if(type_checker){
				asc_error(expr->line, expr->col, "null can only be compared with mapping, array, or record types", asc);}
                return getBoolType;
            }
        }

        if (!lNullCheck && rNullCheck) {
            typeKind curKind = getLType->type->kind;
            bool recArrFun = (curKind == TYPE_MAPPING || curKind == TYPE_ARRAY || curKind == TYPE_RECORD);
            if (recArrFun) {
                expr->resolved_type = getBoolType;
                return getBoolType;
            } else {
	                      expr->resolved_type = getBoolType;
			      if(type_checker){
                asc_error(expr->line, expr->col, "null can only be compared with mapping, array, or record types", asc);
			      }
			      return getBoolType;
            }
        }

        if (get_type(getLType) == get_type(getRType)) {
	  if(type_checker){ expr->resolved_type = getBoolType;}
            return getBoolType;
        }
        else{        expr->resolved_type = getBoolType;
	  if(type_checker){asc_error(expr->line, expr->col, "operands of = must have the same type", asc);}
	}
	return getBoolType;
    }

    default:
      if(type_checker){ asc_error(expr->line, expr->col, "unsupported expression kind", asc);}
        return NULL;
    }

    return NULL;
}

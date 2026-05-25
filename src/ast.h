#ifndef AST_H
#define AST_H

#include <stdbool.h>

#include "../src/symEntry.h"
#include "../src/ir.h"

enum ConstantType {ADDRESS, INTEGER, BOOLEAN, CHARACTER, STRING,NULLTYPE};

typedef struct ConstantValue {
	char* typeName;
	enum ConstantType type;
	int line;
	int col;
	union {
		void* ptr;
		int i;
		bool b;
		char c;
		char* s;
	} value;
}ConstantValue;

typedef struct Basic {
	char* name;
	int line;
	int col;
}Basic;

typedef struct BasicList {
	struct Basic cur;
	struct BasicList *next;
	int count;
        bool isAs;
}BasicList;

typedef struct AstExpr AstExpr;
typedef struct AstStmt AstStmt;
typedef struct AstStmtList AstStmtList;
typedef struct Assignable Assignable;
typedef struct Basic Basic;
typedef struct Expr Expr;
typedef struct IfPrefix IfPrefix;

typedef enum {
    AST_EXPR_CONST,
    AST_EXPR_ASSIGNABLE,

    AST_EXPR_NEG,
    AST_EXPR_NOT,

    AST_EXPR_ADD,
    AST_EXPR_SUB,
    AST_EXPR_MUL,
    AST_EXPR_DIV,
    AST_EXPR_REM,

    AST_EXPR_AND,
    AST_EXPR_OR,

    AST_EXPR_LT,
    AST_EXPR_EQ,

    AST_EXPR_PAREN,

    AST_EXPR_MEMOP
} AstExprKind;

typedef enum {
    AST_STMT_ASSIGN,
    AST_STMT_RETURN,
    AST_STMT_IF,
    AST_STMT_BLOCK
} AstStmtKind;


struct AstExpr {
    AstExprKind kind;
    int line;
    int col;

    symEntry *resolved_type;

    union {
        struct ConstantValue *constant;

        struct Basic *assignable;

     
        struct {
            AstExpr *child;
        } unary;

        struct {
            AstExpr *left;
            AstExpr *right;
        } binary;
    } data;
};


struct AstStmtList {
    AstStmt *stmt;
    AstStmtList *next;
};


struct AstStmt {
    AstStmtKind kind;
    int line;
    int col;

    union {
        struct {
            struct Assignable *target;
            AstExpr *value;
        } assign_stmt;

        struct {
            AstExpr *condition;
            AstStmtList *then_block;
            AstStmtList *else_block;
        } if_stmt;

       struct {
	 AstStmtList *statements;
	 symbolTable *scope;
       } block_stmt;
      struct{
	struct symEntry *range;
	AstExpr *target;
      } return_stmt;
      
    } data;
};
typedef enum ExprType {EXPR_INT, EXPR_BOOL, EXPR_CHAR, EXPR_ADDR} ExprType;

struct Expr {
    typeExpr *type_info;
    symEntry* temp_var;
    AstExpr *astExpr;
    ExprType expr_type;
    BPList* true_list; //these only matter if expr_bool for if and while statements
    BPList* false_list;
    int line;
    int col;
};

struct ExprList {
        struct Expr cur;
        struct ExprList *next;
        int count;
	int line;
	int col;
};
typedef struct ExprList ExprList;

typedef struct Statement {
  AstStmt *astStmt;
  AstStmtList *astStmtList;
  BPList* next_list;
} Statement;

enum AssignableType {ASS_ID, FUNC_CALL, ARRAY_ACCESS, RECORD_ACCESS, ARRAY_DIM_LOOKUP};

struct Assignable {
    enum AssignableType type;
    typeExpr *type_info;
    int line;
    int col;
    symEntry *resolved_type;
    struct Basic basic;
    union {
        char* id;
        struct {
            char* name;
            ExprList arg_list;
        } func_call;
        //TODO: figure out array/record access/array dimension lookup stuff
        struct {
            char *arr_name;
            Expr idx;
        } arr_access;
        struct {
            char *rec_name;
            char *member_name;
        } rec_access; // overloaded with both record access and array dim lookup
    } val;
};

typedef struct IfPrefix {
    AstExpr *expr;
    BPList *true_list;
    BPList *false_list;
    int line;
    int col;
    int mInstr1;
    Statement sblock1;
    BPList *nextList;
} IfPrefix;
AstExpr *ast_new_reserve(struct Assignable *assignable, symEntry *addressEntry);
AstExpr *ast_new_const_expr(struct ConstantValue *constant);

AstExpr *ast_new_assignable_expr(struct Assignable *assignable);

AstExpr *ast_new_unary_expr(
    AstExprKind kind,
    AstExpr *child,
    int line,
    int col
);

AstExpr *ast_new_binary_expr(
    AstExprKind kind,
    AstExpr *left,
    AstExpr *right,
    int line,
    int col
);

AstStmt *ast_new_assign_stmt(
    struct Assignable *assignable,
    AstExpr *value,
    int line,
    int col
);

AstStmt *ast_new_return_stmt(
    AstExpr *target,
    symEntry *range,
    int line,
    int col
);

AstStmt *ast_new_if_stmt(
    AstExpr *condition,
    AstStmt *then_block,
    AstStmt *else_block,
    int line,
    int col
);
typeExpr *normalize_type_expr(typeExpr *t);
AstStmt *ast_new_block_stmt(AstStmtList *statements, symbolTable *scope, int line, int col);

AstStmtList *ast_new_stmt_list(AstStmt *stmt);

AstStmtList *ast_stmt_list_append(AstStmtList *list, AstStmt *stmt);

bool is_dot_op_type_safe(Assignable *parent,
                         Assignable *LHChild,
                         Basic *RHChild);

bool is_ablock_type_safe(Assignable *parent, Assignable *id, ExprList *block);

bool is_mem_op_type_safe(Expr *parent,
                         Assignable *child,
                         symEntry *parent_target);

bool is_while_stmt_type_safe(Expr *cond, symEntry *cond_target);

symEntry *typecheck_expr(AstExpr *expr, FILE *asc,symbolTable *table,bool type_checker);

void typecheck_stmt_list(AstStmtList *list, FILE *asc,symbolTable *table, bool type_checker);

bool typecheck_stmt(AstStmt *stmt, FILE *asc,symbolTable *table,bool type_checker);

#endif

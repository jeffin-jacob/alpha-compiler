%{
    
    #include "../src/ast_test_defs.h"
    #include "../src/symbol-table.h"
    #include "../src/ir.h"
    #include "../src/ast.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #define MAX_MSG_LEN 100

    extern int yylex();
    extern char *yytext;
    extern FILE *asc_file;
    extern int tok_line;
    extern int tok_col;
    extern bool debug;
    extern bool debugflag;

    //TODO: make this variable false if typechecking fails
    extern bool intermediate;

    /* set true to enable typechecking,
       and false to disable typechecking */
    extern bool is_typechecking;

    extern bool type_checker;
    extern bool caughtErrors;
    extern IrInstr ir_array[];
    extern int ir_array_idx;
    int tempvarnum;

    char* new_temp_var_name() {
        char* b = malloc(20*sizeof(char));
        sprintf(b, "$%d", tempvarnum);
        tempvarnum++;
        return b;
    }

static void asc_error(int line, int col, const char *msg) {
    if (asc_file)
        fprintf(asc_file, "LINE %03d:%d ** ERROR: %s\n", line, col, msg);
    fprintf(stderr, "LINE %03d:%d ** ERROR: %s\n", line, col, msg);
}

static void asc_tc_error(int line, int col, char *msg) {
    /* if (is_typechecking) { */
    /*     intermediate = false; */
        asc_error(line, col, msg);
    /* } */
}

    void yyerror(char const*s);

    //setup symbol table
    extern symbolTable* headTable;
    extern symbolTable* curTable;
    bool sblock_open_scope = true;
    symEntry *current_function = NULL;
    extern int local_offset;

    /* Quickly generate temp variable and add it to symbol table if need be */
    symEntry *new_tmp_var(symEntry *type_entry, bool is_update_table) {
        char *tmp_name = new_temp_var_name();
        typeExpr *type_wrapper = type_single(type_entry);
        symEntry *ret_entry = sym_make_local(tmp_name, type_wrapper);
        if (is_update_table) {
            addEntry(curTable, ret_entry);
        }
	if(get_type(type_entry) -> kind == TYPE_MAPPING){
	  ret_entry -> isFuncPtr = true;
	}
        return ret_entry;
    }

    /* Calculates byte index */
    IrOp ir_array_preprocessing(symEntry *idx, symEntry *elem_type) {
        symEntry *int_entry = getEntryInSymbolTable(curTable, "integer", true);
        IrOp ir_idx = ir_op_from_symEntry(idx);
        IrOp ir_elem_size = ir_op_from_int((int)(get_size(elem_type)));
        IrOp x = ir_op_from_symEntry(new_tmp_var(int_entry, false));
        ir_emit_assign(x, ir_elem_size);
        IrOp ir_initial_shamt = ir_op_from_int(8);
        IrOp y = ir_op_from_symEntry(new_tmp_var(int_entry, false));
        ir_emit_assign(y, ir_initial_shamt);
        IrOp tmp_res1 = ir_op_from_symEntry(new_tmp_var(int_entry, false));
        IrOp tmp_res2 = ir_op_from_symEntry(new_tmp_var(int_entry, false));
        ir_emit_mul(tmp_res1, ir_idx, x);
        ir_emit_add(tmp_res2, tmp_res1, y);
        return tmp_res2;
    }

    /* Generates IR for bounds checking */
    void ir_array_bounds_check(symEntry *arr, symEntry *idx) {
        symEntry *int_entry = getEntryInSymbolTable(curTable, "integer", true);
        IrOp ir_tmp1 = ir_op_from_symEntry(new_tmp_var(int_entry, false));      
        IrOp ir_arr = ir_op_from_symEntry(arr);
        IrOp ir_zero = ir_op_from_int(0);
        IrOp x = ir_op_from_symEntry(new_tmp_var(int_entry, false));
        ir_emit_assign(x, ir_zero);	
        ir_emit_array_read(ir_tmp1, ir_arr, x);
        symEntry *bool_entry = getEntryInSymbolTable(curTable, "Boolean", true);
        IrOp ir_tmp2 = ir_op_from_symEntry(new_tmp_var(bool_entry, false));      
        IrOp ir_idx = ir_op_from_symEntry(idx);
        ir_emit_lt(ir_tmp2, ir_idx, ir_tmp1);
        BPList* true_list  = newBPList(ir_array_idx);
        ir_emit_if_true_bp(ir_tmp2);
        ir_emit_seg_fault();
        backpatch(true_list, ir_array_idx);
    }

%}
//Needed so ts shows up in grammar.tab.h and lexer doesnt complain
%code requires {
    #include <stdlib.h>
    #include <stdbool.h>
    #include "../src/symbol-table.h"
    #include "../src/ast.h"
   
    struct SblockScope {
        bool dont_open_scope;
    };
}

//union
%union {
    struct AstStmt *stmt;
    struct AstStmtList *stmt_list;
    struct AstExpr *expr;
    struct Basic basic;
    struct BasicList bl;

    struct ConstantValue k;
    struct BasicList p;

    struct Expr e;
    struct ExprList el;

    struct Statement s;

    struct Assignable ab;
    struct SblockScope sbl_sc;
    struct IfPrefix ip;
    int m_instr; //M.instr from book fig 6.43 for infix rule backpatch stuff in &s and |s
    BPList* n_next_list; //N.nextlist from book fig 6.46 for backpatch compound statements

    char *type_str;
}
%define parse.error verbose
// identifier
%token ID 101

// type names
%token T_INTEGER   201
%token T_ADDRESS   202
%token T_BOOLEAN   203
%token T_CHARACTER 204
%token T_STRING    205

// constants (literals)
%token  <k> C_INTEGER   301
%token   <k> C_NULL      302
%token   <k> C_CHARACTER 303
%token   <k> C_STRING    304
%token   <k> C_TRUE      305
%token   <k> C_FALSE     306

// other keywords
%token WHILE       401
%token IF          402
%token THEN        403
%token ELSE        404
%token TYPE        405
%token FUNCTION    406
%token RETURN      407
%token EXTERNAL    408
%token AS          409

// punctuation - grouping
%token L_PAREN     501
%token R_PAREN     502
%token L_BRACKET   503
%token R_BRACKET   504
%token L_BRACE     505
%token R_BRACE     506
// punctuation - other
%token SEMI_COLON  507
%token COLON       508
%token COMMA       509
%token ARROW       510

// operators
%token ADD         601
%token SUB_OR_NEG  602
%token MUL         603
%token DIV         604
%token REM         605
%token LESS_THAN   606
%token EQUAL_TO    607
%token ASSIGN      608
%token NOT         609
%token AND         610
%token OR          611
%token DOT         612
%token RESERVE     613
%token RELEASE     614

// erroneous tokens
%token FD_UP_STRING 701

//added for comments
%token COMMENT     700
%token COMMENT_EOF 702
//added for now, it is what it is
%token UNDEFINED_INPUT 800

//Precedence rules
%right ASSIGN
%left OR
%left AND
%nonassoc EQUAL_TO LESS_THAN
%left ADD SUB_OR_NEG
%left MUL DIV REM
%right NOT
%right NEG
%left DOT
%right RESERVE RELEASE


%type <basic> ID
%type <basic> T_INTEGER
%type <basic> T_ADDRESS
%type <basic> T_BOOLEAN
%type <basic> T_CHARACTER
%type <basic> T_STRING

%type <basic> DOT
%type <basic> recOp

%type <basic> RESERVE
%type <basic> RELEASE
%type <basic> memOp

%type <ip> WHILE_PREFIX
%type <basic> WHILE

%type <basic> L_PAREN

%type <k> constant
%type <bl> idlist
%type <bl> parameter
%type <type_str> type_id

%type <e> expression
%type <el> argumentList
%type <el> ablock
%type <ab> assignable
%type <ip> if_first
%type <ip> if_initial
%type <ip> if_then
%type <ip> if_prefix
%type <s> if_stmt
%type <s> simple_statement
%type <s> statement_list
%type <s> compound_statement
%type <s> sblock

%start program
%%

program:
    prototype_or_definition_list { ir_emit_end(); }
    ;

prototype_or_definition_list:
                            prototype prototype_or_definition_list
                            | definition prototype_or_definition_list
                            | prototype
                            | definition
                            ;
prototype:
         FUNCTION ID COLON ID
         {
             symEntry *type_entry = getEntryInSymbolTable(curTable, $4.name, true);
             typeExpr *fn_type;
             if (type_entry == NULL) {
                 if (debug || debugflag) fprintf(stderr, "Type \"%s\" is undefined!\n", $4.name);
                 fn_type = type_undefined();
             } else {
                 fn_type = type_single(type_entry);
             }
             symEntry *fn_entry = sym_make_function($2.name, fn_type);
             addEntry(curTable, fn_entry);
         }
         | EXTERNAL FUNCTION ID COLON ID
         {
             symEntry *type_entry = getEntryInSymbolTable(curTable, $5.name, true);
             typeExpr *fn_type;
             if (type_entry == NULL) {
                 if (debug || debugflag) fprintf(stderr, "Type \"%s\" is undefined!\n", $5.name);
                 fn_type = type_undefined();
             } else {
                 fn_type = type_single(type_entry);
             }
             symEntry *fn_entry = sym_make_function($3.name, fn_type);
             addEntry(curTable, fn_entry);
         }
         ;





constant:
        C_INTEGER {
	  $$ = $1;
	  }
        | C_NULL {
	  $$ = $1;
	  }
        | C_CHARACTER {
	  $$ = $1;
	  }
        | C_STRING {
	  $$ = $1;
	  }
        | C_TRUE {
	  $$ = $1;
	  }
        | C_FALSE {
	  $$ = $1;
	  }
        ;

recOp:
     DOT {$$ = $1;}
     ;

assignOp:
        ASSIGN {}
        ;

memOp:
     RESERVE {$$ = $1;}
     | RELEASE {$$ = $1;}
     ;

expression:
          constant {

	    ConstantValue *kcopy = malloc(sizeof(ConstantValue));
              *kcopy = $1;
              $$.astExpr = ast_new_const_expr(kcopy);
        char* temp_name = new_temp_var_name();
        symEntry* type_entry;
        switch ($1.type) {
        case INTEGER:
            type_entry = getEntryInSymbolTable(curTable, "integer", true);
            break;
        case BOOLEAN:
            type_entry = getEntryInSymbolTable(curTable, "Boolean", true);
            break;
        case CHARACTER:
            type_entry = getEntryInSymbolTable(curTable, "character", true);
            break;
        case STRING:
            type_entry = getEntryInSymbolTable(curTable, "string", true);
            break;
        case ADDRESS:
            type_entry = getEntryInSymbolTable(curTable, "address", true);
            break;
	default:
	  break;
        }
        symEntry *entry = sym_make_local(temp_name, type_single(type_entry));
         
        $$.temp_var = entry;
        addEntry(curTable, entry);
	
        if (intermediate) {
            IrOp x = ir_op_from_symEntry(entry);
            IrOp y;
	    if (debugflag) {printf("immediate typing: %d \n", $1.type); }
            switch ($1.type) {
            case INTEGER:
                $$.expr_type = EXPR_INT;
                y = ir_op_from_int($1.value.i);
                ir_emit_assign(x, y);
                break;
            case BOOLEAN:
	        y = ir_op_from_bool($1.value.b);
		ir_emit_assign(x, y);
		
		$$.expr_type = EXPR_BOOL;
                /* $$.true_list = NULL; */
                /* $$.false_list = NULL; */

                /* // generate goto for true/false like production B->true and B->false in textbook fig 6.43 */
                /* if ($1.value.b) { */
		/*   $$.true_list = newBPList(ir_array_idx); */
                /* } else { */
		/*   $$.false_list = newBPList(ir_array_idx); */
                /* } */
                /* ir_emit_goto_bp(); */
		//y = ir_op_from_bool($1.value.b);
                break;
            case CHARACTER:
                y = ir_op_from_char($1.value.c);
                ir_emit_assign(x, y);
                break;

            case STRING:
              char *curString = $1.value.s;
              int length = strlen(curString);
              IrOp lengthOp = ir_op_from_int(length);
              IrOp lengthOp8 = ir_op_from_int(length + 8);
              symEntry *charEntry = getEntryInSymbolTable(curTable, "character", true);
              symEntry *intEntry = getEntryInSymbolTable(curTable, "integer", true);
              symEntry *reserve = getEntryInSymbolTable(curTable, "reserve", true);
              IrOp ir_reserve = ir_op_from_symEntry(reserve);
              symEntry *plus8Entry =  sym_make_local(new_temp_var_name(), type_single(intEntry));
              IrOp plus8Op = ir_op_from_symEntry(plus8Entry);
              ir_emit_assign(plus8Op,lengthOp8);
              ir_emit_param(plus8Op);
              ir_emit_call(x,ir_reserve,1); //x should be reserved, now we have to assign each index in a loop
              symEntry *lengthEntry =  sym_make_local(new_temp_var_name(), type_single(intEntry));
              symEntry *zeroEntry =  sym_make_local(new_temp_var_name(), type_single(intEntry));
              IrOp zeroOp = ir_op_from_int(0);
              ir_emit_assign(ir_op_from_symEntry(zeroEntry),zeroOp);
              ir_emit_assign(ir_op_from_symEntry(lengthEntry),lengthOp);
              ir_emit_array_write(x,ir_op_from_symEntry(zeroEntry),ir_op_from_symEntry(lengthEntry));
              for (int i = 0; i < length; i++) {
                symEntry *idx_tmp =  sym_make_local(new_temp_var_name(), type_single(intEntry));
                symEntry *char_tmp = sym_make_local(new_temp_var_name(), type_single(charEntry));
                if (debugflag || debug) { printf("strIndex: %d \n", i); }
                if (debugflag || debug) { printf("strChar: %c \n", curString[i]); }
                ir_emit_assign(ir_op_from_symEntry(idx_tmp),ir_op_from_int(i+ 8));

                ir_emit_assign(ir_op_from_symEntry(char_tmp),ir_op_from_char(curString[i]));

                //IrOp ir_byte_idx = ir_array_preprocessing(idx_tmp, charEntry);

                ir_emit_array_write(x,ir_op_from_symEntry(idx_tmp),ir_op_from_symEntry(char_tmp));
              }

            default:
                //TODO: implement strings (and addresses? maybe not, address literals arent a thing in the spec so maybe the address type is spillover from an older version of alpha)
                break;
            }
        }
}

          //unary ops
          | SUB_OR_NEG expression %prec NEG {
	    $$.astExpr = ast_new_unary_expr(AST_EXPR_NEG, $2.astExpr, $2.line, $2.col);
	    if (type_checker){
	      //typecheck_expr($$.astExpr,asc_file,curTable);
	    }
	    char* temp_name = new_temp_var_name();
        symEntry* y_entry = $2.temp_var;

        symEntry* x_entry = sym_make_local(temp_name, type_single(getEntryInSymbolTable(curTable, "integer", true)));

        $$.temp_var = x_entry;
        $$.expr_type = EXPR_INT;
        addEntry(curTable, x_entry);
        if (intermediate) {
            IrOp x = ir_op_from_symEntry(x_entry);
            IrOp y = ir_op_from_symEntry(y_entry);
            ir_emit_neg(x, y);
        }
}
          | NOT expression {
        //put type checking stuff here
        //
	    $$.astExpr = ast_new_unary_expr(AST_EXPR_NOT, $2.astExpr, $2.line, $2.col);
            if (type_checker){
              //typecheck_expr($$.astExpr,asc_file,curTable);
            }
	    if (intermediate) {
            //swap true and false lists in not
            $$.expr_type = EXPR_BOOL;
	    char* temp_name = new_temp_var_name();
            symEntry* bool_entry = getEntryInSymbolTable(curTable, "Boolean", true);
            symEntry* x_entry = sym_make_local(temp_name, type_single(bool_entry));
	    $$.temp_var = x_entry;
	    addEntry(curTable, x_entry);

            symEntry* y_entry = $2.temp_var;
            
            IrOp x = ir_op_from_symEntry(x_entry);
            IrOp y = ir_op_from_symEntry(y_entry);
            
            ir_emit_not(x, y);
            //$$.true_list = $2.false_list;
            //$$.false_list = $2.true_list;
        }
}

          //binary ops
          | expression ADD expression {
        char* temp_name = new_temp_var_name();
        symEntry* y_entry = $1.temp_var;
        symEntry* z_entry = $3.temp_var;
        //put type checking stuff here
        //
          $$.astExpr = ast_new_binary_expr(AST_EXPR_DIV, $1.astExpr, $3.astExpr, $1.line, $1.col);
            if (type_checker){
              //typecheck_expr($$.astExpr,asc_file,curTable);
            }
	  symEntry* x_entry = sym_make_local(temp_name, type_single(getEntryInSymbolTable(curTable, "integer", true)));
        $$.temp_var = x_entry;
        $$.expr_type = EXPR_INT;
        addEntry(curTable, x_entry);
        if (intermediate) {
            IrOp x = ir_op_from_symEntry(x_entry);
            IrOp y = ir_op_from_symEntry(y_entry);
            IrOp z = ir_op_from_symEntry(z_entry);
            ir_emit_add(x, y, z);
        }
}
          | expression SUB_OR_NEG expression {
	              
        char* temp_name = new_temp_var_name();
        symEntry* y_entry = $1.temp_var;
        symEntry* z_entry = $3.temp_var;
        //put type checking stuff here
        //
                      $$.astExpr = ast_new_binary_expr(AST_EXPR_SUB, $1.astExpr, $3.astExpr, $1.line, $1.col);
            if (type_checker){
            }
		      symEntry* x_entry = sym_make_local(temp_name, type_single(getEntryInSymbolTable(curTable, "integer", true)));
        $$.temp_var = x_entry;
        $$.expr_type = EXPR_INT;
        addEntry(curTable, x_entry);
        if (intermediate) {
            IrOp x = ir_op_from_symEntry(x_entry);
            IrOp y = ir_op_from_symEntry(y_entry);
            IrOp z = ir_op_from_symEntry(z_entry);
            ir_emit_sub(x, y, z);
        }
}
          | expression MUL expression {
	
        char* temp_name = new_temp_var_name();
        symEntry* y_entry = $1.temp_var;
        symEntry* z_entry = $3.temp_var;
        //put type checking stuff here
        //
	$$.astExpr = ast_new_binary_expr(AST_EXPR_MUL, $1.astExpr, $3.astExpr, $1.line, $1.col);
            if (type_checker){
              //typecheck_expr($$.astExpr,asc_file,curTable);
            }
	symEntry* x_entry = sym_make_local(temp_name, type_single(getEntryInSymbolTable(curTable, "integer", true)));
        $$.temp_var = x_entry;
        $$.expr_type = EXPR_INT;
        addEntry(curTable, x_entry);
        if (intermediate) {
            IrOp x = ir_op_from_symEntry(x_entry);
            IrOp y = ir_op_from_symEntry(y_entry);
            IrOp z = ir_op_from_symEntry(z_entry);
            ir_emit_mul(x, y, z);
        }
}
          | expression DIV expression {

	  $$.astExpr = ast_new_binary_expr(AST_EXPR_DIV, $1.astExpr, $3.astExpr, $1.line, $1.col);
            if (type_checker){
              //typecheck_expr($$.astExpr,asc_file,curTable);
            }
	  char* temp_name = new_temp_var_name();
        symEntry* y_entry = $1.temp_var;
        symEntry* z_entry = $3.temp_var;
        //put type checking stuff here
        //

        symEntry* x_entry = sym_make_local(temp_name, type_single(getEntryInSymbolTable(curTable, "integer", true)));
        $$.temp_var = x_entry;
        $$.expr_type = EXPR_INT;
        addEntry(curTable, x_entry);
        if (intermediate) {
            IrOp x = ir_op_from_symEntry(x_entry);
            IrOp y = ir_op_from_symEntry(y_entry);
            IrOp z = ir_op_from_symEntry(z_entry);
            ir_emit_div(x, y, z);
        }
}
          | expression REM expression {
	    $$.astExpr = ast_new_binary_expr(AST_EXPR_REM, $1.astExpr, $3.astExpr, $1.line, $1.col);
            if (type_checker){
              //typecheck_expr($$.astExpr,asc_file,curTable);
            }
	    char* temp_name = new_temp_var_name();
        symEntry* y_entry = $1.temp_var;
        symEntry* z_entry = $3.temp_var;
        //put type checking stuff here
        //

        symEntry* x_entry = sym_make_local(temp_name, type_single(getEntryInSymbolTable(curTable, "integer", true)));
        $$.temp_var = x_entry;
        $$.expr_type = EXPR_INT;
        addEntry(curTable, x_entry);
        if (intermediate) {
            IrOp x = ir_op_from_symEntry(x_entry);
            IrOp y = ir_op_from_symEntry(y_entry);
            IrOp z = ir_op_from_symEntry(z_entry);
            ir_emit_rem(x, y, z);
        }
}
          | expression AND <m_instr>{$$ = ir_array_idx;} expression { 
          //$1 is expr1 $4 is expr2, $3 is m_instr (index of next instruction after first expression)
            $$.astExpr = ast_new_binary_expr(AST_EXPR_AND, $1.astExpr, $4.astExpr, $1.line, $1.col);
        //type checking stuff here yada yada
        //
            if (type_checker){
              //typecheck_expr($$.astExpr,asc_file,curTable);
            }
        if (intermediate) {
            $$.expr_type = EXPR_BOOL;
	    char* temp_name = new_temp_var_name();
            symEntry* bool_entry = getEntryInSymbolTable(curTable, "Boolean", true);
            symEntry* x_entry = sym_make_local(temp_name, type_single(bool_entry));
            addEntry(curTable, x_entry);
	    $$.temp_var = x_entry;
            symEntry* y_entry = $1.temp_var;
            symEntry* z_entry = $4.temp_var;

            IrOp x = ir_op_from_symEntry(x_entry);
            IrOp y = ir_op_from_symEntry(y_entry);
            IrOp z = ir_op_from_symEntry(z_entry);

            ir_emit_and(x, y, z);
            //backpatch($1.true_list, $3);
            //$$.true_list = $4.true_list;
            //$$.false_list = mergeBPLists($1.false_list, $4.false_list);
        }
}
          | expression OR <m_instr>{$$ = ir_array_idx;} expression { //similar to and
            $$.astExpr = ast_new_binary_expr(AST_EXPR_OR, $1.astExpr, $4.astExpr, $1.line, $1.col);
        if (intermediate) {
            $$.expr_type = EXPR_BOOL;
	    char* temp_name = new_temp_var_name();
            symEntry* bool_entry = getEntryInSymbolTable(curTable, "Boolean", true);
            symEntry* x_entry = sym_make_local(temp_name, type_single(bool_entry));
            addEntry(curTable, x_entry);
	    $$.temp_var = x_entry;
            symEntry* y_entry = $1.temp_var;
            symEntry* z_entry = $4.temp_var;

            IrOp x = ir_op_from_symEntry(x_entry);
            IrOp y = ir_op_from_symEntry(y_entry);
            IrOp z = ir_op_from_symEntry(z_entry);

            ir_emit_or(x, y, z);

            //backpatch($1.false_list, $3);
            //$$.false_list = $4.false_list;
            //$$.true_list = mergeBPLists($1.true_list, $4.true_list);
        }
}
          | expression LESS_THAN expression {
            $$.astExpr = ast_new_binary_expr(AST_EXPR_LT, $1.astExpr, $3.astExpr, $1.line, $1.col);
        //type checking stuff, make sure expressions are same type for compare
        //
	                if (type_checker){
			  //typecheck_expr($$.astExpr,asc_file,curTable);
            }
        if (intermediate) {
            //new bool entry since we aint doing the infix compare thingy
            char* temp_name = new_temp_var_name();
            symEntry* bool_entry = getEntryInSymbolTable(curTable, "Boolean", true);
            symEntry* x_entry = sym_make_local(temp_name, type_single(bool_entry)); 
            addEntry(curTable, x_entry);
	    $$.temp_var = x_entry;
            symEntry* y_entry = $1.temp_var;
            symEntry* z_entry = $3.temp_var;

            IrOp x = ir_op_from_symEntry(x_entry);
            IrOp y = ir_op_from_symEntry(y_entry);
            IrOp z = ir_op_from_symEntry(z_entry);

            ir_emit_lt(x, y, z);

            $$.expr_type = EXPR_BOOL;
            //$$.true_list = newBPList(ir_array_idx);
            //ir_emit_if_true_bp(x);
            //$$.false_list = newBPList(ir_array_idx);
            //ir_emit_if_false_bp(x);
        }
}
          | expression EQUAL_TO expression {
        //type checking stuff, make sure expressions are same type for compare
        //
	    $$.astExpr = ast_new_binary_expr(AST_EXPR_EQ, $1.astExpr, $3.astExpr, $1.line, $1.col);
            if (type_checker){
              //typecheck_expr($$.astExpr,asc_file,curTable);
            }
	    if (intermediate) {
            //new bool entry since we aint doing the infix compare thingy
            char* temp_name = new_temp_var_name();
            symEntry* bool_entry = getEntryInSymbolTable(curTable, "Boolean", true);
            symEntry* x_entry = sym_make_local(temp_name, type_single(bool_entry)); 
            addEntry(curTable, x_entry);
	    $$.temp_var = x_entry;
            symEntry* y_entry = $1.temp_var;
            symEntry* z_entry = $3.temp_var;

            IrOp x = ir_op_from_symEntry(x_entry);
            IrOp y = ir_op_from_symEntry(y_entry);
            IrOp z = ir_op_from_symEntry(z_entry);

            ir_emit_eq(x, y, z);

            $$.expr_type = EXPR_BOOL;
            //$$.true_list = newBPList(ir_array_idx);
            //ir_emit_if_true_bp(x);
            //$$.false_list = newBPList(ir_array_idx);
            //ir_emit_if_false_bp(x);
        }

}

          | assignable {
	    
    $$.astExpr = ast_new_assignable_expr(&$1);
	                if (type_checker){
			  //typecheck_expr($$.astExpr,asc_file,curTable);
            }

	switch($1.type) {
        case ASS_ID:
            if (intermediate) {
	        symEntry *lookForTemp =  getEntryInSymbolTable(curTable, $1.val.id, true);
		if(lookForTemp == NULL){
		  intermediate = false;
		  caughtErrors = true;
		  asc_tc_error($1.line, $1.col, "assignable was not declared/no type");
		  if (debugflag) { printf("ASS_ID: %s \n", $1.val.id); }
		  $$.astExpr -> resolved_type = NULL;
		  break;
		}
                $$.temp_var = getEntryInSymbolTable(curTable, $1.val.id, true);
		typeExpr *tempType = get_type($$.temp_var);
		typeKind tempKind = tempType -> kind;
		if (debugflag) { printf("ASS_ID: %s \n", $$.temp_var -> name); }
		switch(tempKind){
		      case TYPE_MAPPING:
		      case TYPE_RECORD:
		      case TYPE_ARRAY:
			if (debugflag) { printf("setting expr_type to EXPR_ADDR \n"); }
			$$.expr_type = EXPR_ADDR;
			break;
		    case TYPE_PRIMITIVE:
		      primitives typePrim = tempType -> as.prim;
		      switch(typePrim){
		         case PRIM_BOOL:
			    $$.expr_type = EXPR_BOOL;
			    /*$$.true_list = newBPList(ir_array_idx);
			    IrOp x = ir_op_from_symEntry($$.temp_var);
			    ir_emit_if_true_bp(x);
			    $$.false_list = newBPList(ir_array_idx);
			    ir_emit_if_false_bp(x); */
			    break;
		         case PRIM_INT:
			   $$.expr_type = EXPR_INT;
			   break;
		         case PRIM_CHAR:
			   $$.expr_type = EXPR_CHAR;
			   break;
		      default:
			break;
		      }
                      break;
		default:
		  break;
		}
		/*
                if (!strcmp($$.temp_var->type->as.single->name, "Boolean")) { //this could probably be done cleaner later
                    $$.expr_type = EXPR_BOOL;
                    $$.true_list = newBPList(ir_array_idx);
                    IrOp x = ir_op_from_symEntry($$.temp_var);
                    ir_emit_if_true_bp(x);
                    $$.false_list = newBPList(ir_array_idx);
                    ir_emit_if_false_bp(x);
		    } */
            }
            break;
        case FUNC_CALL:
            symEntry *fn_entry = getEntryInSymbolTable(curTable, $1.val.func_call.name, true);
            //type check the function

            if (intermediate) {
	            char* temp_name = new_temp_var_name();
                symEntry* fn_range_type_entry = function_get_range(fn_entry);
                symEntry* result_entry = sym_make_local(temp_name, type_single(fn_range_type_entry)); 
                //!!!TODO: functions only return either integers or booleans
                addEntry(curTable, result_entry);
                $$.temp_var = result_entry;
                $$.expr_type = EXPR_INT;

                ExprList* current_arg = &$1.val.func_call.arg_list;
                while (current_arg != NULL) {
                    IrOp p = ir_op_from_symEntry(current_arg->cur.temp_var);
                    ir_emit_param(p);
                    current_arg = current_arg->next;
                }
                IrOp result = ir_op_from_symEntry(result_entry);
                IrOp fn = ir_op_from_symEntry(fn_entry);
		
		if (fn_entry->isFuncPtr) {
		  //printf("should be trying to emit an indirect call \n");
		  ir_emit_call_indirect(result, fn, $1.val.func_call.arg_list.count);
		} else {
		  ir_emit_call(result, fn, $1.val.func_call.arg_list.count);
		}
                if (!strcmp(fn_range_type_entry->name, "Boolean")) { //this should also probably be done cleaner later
                    /* $$.expr_type = EXPR_BOOL;
                    $$.true_list = newBPList(ir_array_idx);
                    IrOp x = ir_op_from_symEntry($$.temp_var);
                    ir_emit_if_true_bp(x);
                    $$.false_list = newBPList(ir_array_idx);
                    ir_emit_if_false_bp(x); */
                }
            }
            break;
        case ARRAY_ACCESS:
            symEntry *arr_entry = 
                            getEntryInSymbolTable(curTable, 
                                                  $1.val.arr_access.arr_name, 
                                                  true);

            /* type check the array */

            if (intermediate) {
                /* IR-gen for array reads */
                symEntry *idx_entry = $1.val.arr_access.idx.temp_var;
                symEntry *elem_type = (get_type(arr_entry))->as.arr.elem;
                ir_array_bounds_check(arr_entry, idx_entry);
                IrOp tmp_res2 = ir_array_preprocessing(idx_entry, elem_type);
                symEntry *tmp_entry = new_tmp_var(elem_type, true);
                $$.temp_var = tmp_entry;
                /* TODO populate $$.expr_type */
                IrOp ir_tmp = ir_op_from_symEntry(tmp_entry);
                IrOp ir_arr = ir_op_from_symEntry(arr_entry);
                ir_emit_array_read(ir_tmp, ir_arr, tmp_res2);
            }
            break;
        case RECORD_ACCESS:
            char *rec_name = $1.val.rec_access.rec_name;
            char *member_name = $1.val.rec_access.member_name;
            symEntry *rec = getEntryInSymbolTable(curTable, rec_name, true);
            symbolTable *rec_table = (get_type(rec))->as.rec.scope;
            symEntry *member = getEntryInSymbolTable(rec_table, member_name, false);

            /* type check the record */

            if (intermediate) {
                /* IR-gen for record reads */
	      symEntry *intEntry = getEntryInSymbolTable(curTable,"integer",true);
                symEntry *member_type = member->type->as.single;
                symEntry *tmp_var = new_tmp_var(member_type, true);
                $$.temp_var = tmp_var;
                /* TODO populate $$.expr_type */
                IrOp ir_rec = ir_op_from_symEntry(rec);
                IrOp ir_offset = ir_op_from_int(abs(member->offset));
	        IrOp x = ir_op_from_symEntry(new_tmp_var(intEntry, false));
		ir_emit_assign(x, ir_offset);
                IrOp ir_tmp = ir_op_from_symEntry(tmp_var);
                ir_emit_array_read(ir_tmp, ir_rec, x);
            }
            break;
        case ARRAY_DIM_LOOKUP:
            symEntry *dim_arr_entry = 
                            getEntryInSymbolTable(curTable, 
                                                  $1.val.rec_access.rec_name, 
                                                  true);

            /* type check the array */

            if (intermediate) {
                /* IR-gen for array dimension lookups */
                symEntry *int_entry = getEntryInSymbolTable(curTable, "integer", 
                                                            true);
                char *name_arr = $1.val.rec_access.member_name;
                char dim_char = name_arr[strlen(name_arr) - 1]; 
                int dim = (int)dim_char - 0x30;
                symEntry *dim_tmp = new_tmp_var(int_entry, true);
                $$.temp_var = dim_tmp;
                IrOp ir_dim_tmp = ir_op_from_symEntry(dim_tmp);
                /* currently just handles 1-D arrays */
                if (dim == 0) {
                    $$.expr_type = EXPR_INT;

                    /* TODO make safer */
                    symEntry *arr_single = dim_arr_entry->type->as.single;
                    int num_dims = arr_single->type->as.arr.dim;

                    IrOp ir_num_dims = ir_op_from_int(num_dims);
                    ir_emit_assign(ir_dim_tmp, ir_num_dims);
                } else {
                    $$.expr_type = EXPR_INT;
		    symEntry *offset_tmp = new_tmp_var(int_entry, true);
		    IrOp ir_offset_tmp = ir_op_from_symEntry(offset_tmp);

		    IrOp zeroOp = ir_op_from_int(0);
		    ir_emit_assign(ir_offset_tmp, zeroOp);

		    symEntry *value_tmp = new_tmp_var(int_entry, true);
		    IrOp ir_value_tmp = ir_op_from_symEntry(value_tmp);

		    IrOp ir_dim_arr = ir_op_from_symEntry(dim_arr_entry);

		    ir_emit_array_read(ir_value_tmp, ir_dim_arr, ir_offset_tmp);

		    $$.temp_var = value_tmp;                }
            }
            break;
        default:
            break;
        }
}
          | L_PAREN expression R_PAREN {
	    $$.astExpr = $2.astExpr;
        if (intermediate) {
            $$.temp_var = $2.temp_var;
            $$.expr_type = $2.expr_type;
            $$.true_list = $2.true_list;
            $$.false_list = $2.false_list;
        }
}
          | memOp assignable {//TODO: not doing memops now lol
            symEntry *address_entry = getEntryInSymbolTable(curTable, 
                                                            "address", 
                                                            true);

	    $$.astExpr = ast_new_reserve(&($2),address_entry);
            /* Below conditional faults correct programs. I commented it out
             * while I was implementing ir-gen for reserve and release */
	    //if (address_entry == NULL || 
        //        !is_mem_op_type_safe(&($$), &($2), address_entry)) {
        //        asc_tc_error($1.line, $1.col, "Memory operation expects either "
        //                                      "record or array type");
		//intermediate = false;
		//caughtErrors = true;
		//printf("caught error in memOp assignable \n");
	    //}
            if (intermediate) {
                /* IR-gen for memory operations */
                symEntry *address = getEntryInSymbolTable(curTable, "address", true);
                symEntry *tmp_var = new_tmp_var(address, true);
                $$.temp_var = tmp_var;
                $$.expr_type = EXPR_ADDR;
                IrOp ir_tmp = ir_op_from_symEntry(tmp_var);
                const char *mem_op = $1.name;
                const enum AssignableType type = $2.type;
                if (strcmp(mem_op, "reserve") == 0) {
                    symEntry *reserve = getEntryInSymbolTable(curTable, 
                                                              "reserve", true);
                    IrOp ir_reserve = ir_op_from_symEntry(reserve);                
                    switch (type) {
                        IrOp ir_size;
                        case ASS_ID:
                            if (debugflag) { fprintf(stderr, "PROCESSING ASS_ID WHICH IS RECORD\n"); }
                            symEntry *record = getEntryInSymbolTable(curTable, 
                                                                     $2.val.id, 
                                                                     true);
                            //Was missing the as.single indirection that there usually is
                            symbolTable *rec_scope = record->type->as.single->type->as.rec.scope;  

                            symEntry *curr_entry = NULL;
                            if (rec_scope != NULL) {
                                curr_entry = rec_scope->entry;
                            }
                            int max_offset = 0;
                            while (curr_entry != NULL) {
                                int curr_offset = curr_entry->offset;
                                if (curr_offset > max_offset) {
                                    max_offset = curr_offset;
                                }
                                curr_entry = curr_entry->next;
                            }
                            ir_size = ir_op_from_int(max_offset + 8);
                            symEntry *ints_entry = getEntryInSymbolTable(curTable, "integer", true);
                            //make new symentry thats just an immediate for the call to reserve
                            symEntry *imm_s = new_tmp_var(ints_entry, true);
                            IrOp imm = ir_op_from_symEntry(imm_s);
                            ir_emit_assign(imm, ir_size);
                            ir_emit_param(imm);
                            ir_emit_call(ir_tmp, ir_reserve, 1);
                            break;
                        case ARRAY_ACCESS:
                            symEntry *int_entry = getEntryInSymbolTable(curTable, "integer", true);
                            //make new symentry thats just an immediate '8' for multiply/add
                            symEntry *imm8_s = new_tmp_var(int_entry, true);
                            IrOp imm8 = ir_op_from_symEntry(imm8_s);
                            IrOp v = ir_op_from_int(8);
                            ir_emit_assign(imm8, v);
                            //the size here is in elem count, not memory size
                            symEntry *arr_elem_ct_s = $2.val.arr_access.idx.temp_var;
                            IrOp arr_elem_ct = ir_op_from_symEntry(arr_elem_ct_s);
                            //emit multiply by 8
                            symEntry *arr_tmp_var1_s = new_tmp_var(int_entry, true);
                            IrOp arr_tmp_var1 = ir_op_from_symEntry(arr_tmp_var1_s);
                            ir_emit_mul(arr_tmp_var1, arr_elem_ct, imm8);
                            //emit add by 8
                            symEntry *arr_size_s = new_tmp_var(int_entry, true);
                            IrOp arr_size = ir_op_from_symEntry(arr_size_s);
                            ir_emit_add(arr_size, arr_tmp_var1, imm8);
                            //continue as before
                            ir_emit_param(arr_size);
                            ir_emit_call(ir_tmp, ir_reserve, 1);
                            //make new symentry thats just an immediate '0' for offset
                            symEntry *imm0_s = new_tmp_var(int_entry, true);
                            IrOp imm0 = ir_op_from_symEntry(imm0_s);
                            IrOp v1 = ir_op_from_int(0);
                            ir_emit_assign(imm0, v1);
                            ir_emit_array_write(ir_tmp, imm0, arr_elem_ct);
                            break;
                        default:
                            break;
                    }
                } else if ((strcmp(mem_op, "release") == 0) && 
                           (type == ASS_ID)) {
                    symEntry *release = getEntryInSymbolTable(curTable, 
                                                              "release", true);
                    IrOp ir_release = ir_op_from_symEntry(release);                
                    symEntry *obj = getEntryInSymbolTable(curTable, $2.val.id, 
                                                          true);
                    IrOp ir_obj = ir_op_from_symEntry(obj);
                    ir_emit_param(ir_obj);
                    ir_emit_call(ir_tmp, ir_release, 1);
                }
            }
}
          ;

argumentList:
            expression {
    $$.cur = $1;
    $$.count = 1;
    $$.next = NULL;
    typecheck_expr($1.astExpr,asc_file,curTable,type_checker);
}
            | expression COMMA argumentList{	      
    $$.cur = $1;
    $$.count = $3.count+1;
    $$.next = &$3;
        typecheck_expr($1.astExpr,asc_file,curTable,type_checker);
}
            ;
ablock:
      L_PAREN argumentList R_PAREN {
      $$=$2; $$.line = $1.line; $$.col = $1.col;
      }
      ;

assignable:

          ID {
    $$.type = ASS_ID;
    $$.val.id = $1.name;
        Basic *tmp = malloc(sizeof(Basic));
    
        tmp->name = strdup($1.name);
        tmp -> col = $1.col;
        tmp -> line = $1.line;
    $$.basic = $1;

    symEntry *entry = getEntryInSymbolTable(curTable, $1.name, true); 
    if (entry != NULL) {
        $$.type_info = get_type(entry);
	$$.resolved_type = entry; //-> as.single;
	//printf("assignable name: %s\n", $1.name);
	//printf("assignable type under single def: %p \n", entry -> type -> as.single);
	//printf("assignable type under rec def: %p \n", &(entry -> type -> as.rec));
	
    } else {
        $$.type_info = type_undefined();
    }

    $$.type = ASS_ID;
    $$.val.id = $1.name;
}
          | assignable ablock { //func call or array access
	    //printf("we are in the assignable ablock check 1\n");
        if (!is_ablock_type_safe(&($$), &($1), &($2))) {
            asc_tc_error($2.line, $2.col, "Argument block expects mapping (either "
                                        "function or array) type to immediately "
                                        "precede it");
	    intermediate = false;
	    caughtErrors = true;
	    //printf("caught error in assignable ablock \n");
	}
	//printf("we are in the assignable ablock check 2\n");

        /* TODO discern between array access and function call */
        /* conditional is for testing out-of-bounds array access ir gen */
        Basic curBasic = $1.basic;
        symEntry *entry = getEntryInSymbolTable(curTable, curBasic.name, true);
	//printf("we have the assignable symEntry: %p \n", entry);
	typeExpr *typeInfo = $1.type_info; 
        typeKind curType = typeInfo -> kind;
        annotationKind curAnn; 
        if (entry != NULL) {
            curAnn = entry -> ann;
        }
        /* TODO discern between array access and function call */
        /* conditional is for testing out-of-bounds array access ir gen */
        ExprList el = $2;
       
        if (curType == TYPE_ARRAY) {
            $$.type = ARRAY_ACCESS;
	    typeExpr *paramType = get_type(el.cur.astExpr->resolved_type);
	    typeExpr *intType = get_type(getEntryInSymbolTable(curTable, "integer", true));
	    if(intType != paramType){
	       intermediate = false;
               caughtErrors = true;
	       if(type_checker){
		 
		 asc_tc_error($2.line, $2.col, "array access requires an integer in the ablock");
	       }
	    }
	    $$.type_info = (typeInfo->as).arr.elem -> type;
	    $$.resolved_type = (typeInfo->as).arr.elem;
        }

	else if(curType == TYPE_MAPPING){
            $$.type = FUNC_CALL;
            int curCount = el.count;
            bool asIs = entry -> isAs;
            if(curCount > 1){ //we have to check the asIs
              if(asIs){ //lines up with the asIs
	
               symEntry *domain = function_get_domain(entry);
	       //printf("we have the assignable domain entry: %p \n", domain);
                typeExpr *domType = domain -> type;
		//printf("we have the assignable domain type: %p \n", domType);
		$$.type_info = domType;
		typeKind kind = domType -> kind;
		if(kind != TYPE_RECORD){
		  intermediate = false;
		  caughtErrors = true;
		  //printf("caught error in FUNC_CALL, isAs, curCount > 1, not a record \n");
		 if(type_checker){asc_tc_error($2.line, $2.col, "Function call expects a single argument (was defined without as), got an argument list");}
		}
		else{
		  int entryCount = (domType -> as).rec.entryCount;
		  symbolTable *scope = (domType-> as).rec.scope;
		  if(entryCount != curCount){
		  intermediate = false;
                  caughtErrors = true;
		  //printf("caught error in FUNC_CALL, isAs, record, entryCount not matching up with scopeCount \n");
		  if(type_checker){asc_tc_error($2.line, $2.col, "Function call has a number of arguments mismatch");}
		  }
		  else{
		    symEntry *curEntry = scope -> entry;
		    ExprList *elp = &el;
		    while(curEntry != NULL){
		      symEntry *paramType = (elp -> cur).astExpr -> resolved_type;
		      symEntry *curType = ((curEntry -> type) ->as).single;
		      if(get_type(paramType) != get_type(curType)){
			intermediate = false;
			caughtErrors = true;
			//printf("caught error in FUNC_CALL, isAs, curCount matches, but an element does not match up with record\n");
			if(type_checker){asc_tc_error(el.line, el.col, "type mismatch");}
		      }
		      curEntry = curEntry -> next;
		      elp = elp -> next;
		    }
		symEntry *range = function_get_range(entry);
                typeExpr *rangeType = range -> type;
                $$.type_info = rangeType;
                $$.resolved_type = range;
		  }
		}
              }
              else{
		if(type_checker){asc_tc_error($2.line, $2.col, "Function call expects a single argument (was defined without as), got an argument list");}
                 intermediate = false;
                 caughtErrors = true;
		 //printf("Function call expects a single argument (was defined without as), got an argument list \n");
              }
            }
            else{
                symEntry *domain = function_get_domain(entry);
                //typeExpr *domType = get_type(domain);
                //fprintf(stderr, "dom %p\n", domain);
                symEntry *range = function_get_range(entry);
                //fprintf(stderr, "rng %p\n", range);
		typeExpr *rangeType = range -> type;
		$$.type_info = rangeType;
		$$.resolved_type = range;
		typeExpr *paramType = get_type(el.cur.astExpr->resolved_type);
		//printf("domType: %p \n", domType);
	        //printf("paramType: %p \n", paramType);
		//char *domName = domain->name;
		//char *paramName =  el.cur.astExpr->resolved_type-> name;
		//printf("domName: %s \n", domName);
		//printf("paramName: %s \n", paramName);
                if(get_type(domain) != paramType){
                 intermediate = false;
                 caughtErrors = true;
		 //printf("function call parameter mismatch");
		 //printf("domType: %p \n", domType);
		 //printf("domName: %s \n", domName);
		 //printf("paramType: %p \n", paramType);
		 //printf("paramName: %s \n", paramName);
		 if(type_checker){ asc_tc_error($2.line, $2.col, "function call parameter mismatch!");}
                }
              
            }
	 }
	else{
	    intermediate = false;
            caughtErrors = true;
	    //printf("curAssignable name:  %s \n",  curBasic.name);
	    if(type_checker){ asc_tc_error($1.line, $1.col, "assignable is not of type array or function");}
	}
        char* buf = malloc(128*sizeof(char));
        strcpy(buf, $1.val.id);
        if ($$.type == FUNC_CALL) {
            $$.val.func_call.name = buf;
            $$.val.func_call.arg_list = $2;
        } else if ($$.type == ARRAY_ACCESS) {
            $$.val.arr_access.arr_name = buf;
            $$.val.arr_access.idx = $2.cur;
        } else {
            $$.val.id = buf;
        }
}
          | assignable recOp ID {
    if (!is_dot_op_type_safe(&($$), &($1), &($3))) {
        asc_tc_error($2.line, $2.col, "Dot operator expects either record or "
                                      "array type");
	//printf("Dot operator expects record or array type");
	intermediate = false;
	caughtErrors = true;
    }

    char* buf = malloc(128 * sizeof(char));
    strcpy(buf, $1.val.id);
    $$.val.rec_access.rec_name = buf;
    $$.val.rec_access.member_name = $3.name;
}
          ;
type_id
  : ID {  $$ = $1.name;}
  | T_INTEGER {  $$ = $1.name;}
  | T_ADDRESS {  $$ = $1.name;}
  | T_BOOLEAN {  $$ = $1.name;}
  | T_CHARACTER {  $$ = $1.name;}
  | T_STRING {  $$ = $1.name;}
  ;
declaration
  : type_id COLON ID {
    symEntry *type_entry = getEntryInSymbolTable(curTable, $1, true);
    typeExpr *var_type;
    if (type_entry == NULL) {
        if (debug || debugflag) fprintf(stdout, "Type \"%s\" is undefined!\n", $1);
        var_type = type_undefined();
    } else {
        var_type = type_single(type_entry);
    }
    symEntry *var_entry = sym_make_local($3.name, var_type);
    if(get_type(type_entry)-> kind == TYPE_MAPPING){
      var_entry -> isFuncPtr = true;
      //printf("setting isFuncPtr true for: %s \n", $3.name);
    }
    addEntry(curTable, var_entry);
}
  ;

declaration_list
  : declaration {}
  | declaration SEMI_COLON declaration_list {}
  ;

dblock
  : L_BRACKET declaration_list R_BRACKET {}
  ;

dblock_opt
  : dblock
  | %empty
  ;

idlist
  : ID {$$.cur = $1;
        $$.count = 1;
        $$.next = NULL;}
  | ID COMMA idlist {$$.cur = $1;
                    $$.count = $3.count+1;
                    $$.next = &$3;}
  ;

/* Definition list */

definition: 
          TYPE ID COLON { 
	    //create scope here
	    
	    symbolTable *newChild =  newSymbolTable(curTable, tok_line, tok_col);
	    curTable = newChild;
	  } dblock {//hold onto the current scope as a variable, and then pop into the parent scope of currentScope. Then add the record entry, pointing to that child scope
	    //printf("should be making a record \n");
	    symbolTable *saveCur = curTable;
	    if(!saveCur){
	      //printf("didnt get the scopeTable of the record! \n");
	    }
	    curTable = curTable->parent;
	     if(!saveCur){
	       //printf("didnt pop out of the scopeTable parent! \n");
            }
	     symEntry *saveCurEntry = saveCur ->entry;
	    int recordLength = entry_list_length(saveCur ->entry);
	    typeExpr* newTypeRecord = type_record(recordLength,saveCur);
	    annotationKind newAnn =  ANN_TYPE;
	    symEntry *newRecordEntry = sym_make($2.name, newAnn, newTypeRecord, NULL);
	    addEntry(curTable, newRecordEntry);
	    //printf("made a record type \n");
	    }
            | TYPE ID COLON constant ARROW type_id {//create array typeexpr of num dimensions constant, type type_id, and then sym_make_type
	      if ($4.type != INTEGER) {
	       if (debug || debugflag) fprintf(stderr, "Array dimension must be integer at %d:%d\n", $4.line, $4.col);
	      }
	      else {
	      int dims = $4.value.i;
	      if(dims > 0){
	      symEntry *findTypeEntry = getEntryInSymbolTable(curTable, $6, true);
	      if(!findTypeEntry){}
	      else{
	      typeExpr* newTypeArray = type_array(findTypeEntry,dims);
	      annotationKind newAnn =  ANN_TYPE;
	      symEntry *newArrayEntry = sym_make($2.name,newAnn,newTypeArray,NULL);
	      addEntry(curTable, newArrayEntry);
	      }
	      }
	      }	    
	    
	    }
	    | TYPE ID COLON type_id ARROW type_id {//mapping name ID, type_id -> type_id
	    //printf("making a mapping type \n");
	      symEntry *dom = getEntryInSymbolTable(curTable, $4, true);
	      symEntry *rng = getEntryInSymbolTable(curTable, $6, true);
	      if(dom && rng){
	      typeExpr *newTypeMap = type_mapping(dom,rng);
	      //printf("newTypeMap: %p \n", newTypeMap);
	      annotationKind newAnn =  ANN_TYPE;
	      symEntry *newMapEntry = sym_make($2.name,newAnn,newTypeMap,NULL);
	      addEntry(curTable, newMapEntry);
	      }
	      else{
		//printf("they were not found in the preprocessor or whatever \n");
	      }
	    }
          | ID parameter assignOp {
	    //printf("we are in the function definition \n");
	    symbolTable *newChild =  newSymbolTable(curTable, tok_line, tok_col);
	    curTable = newChild;
        sblock_open_scope = false;
	bool funcCheck = false;
	BasicList curParam = $2;
	bool isAs = curParam.isAs;
        symEntry* f = getEntryInSymbolTable(curTable, $1.name, true);
        //TODO: We need type checking here to make sure its a function rather than if fn exists as well
	if (!f) { intermediate = false; caughtErrors = true; funcCheck = true;}
	else{
	  //printf("checking curAnn\n");
	  annotationKind curAnn = f -> ann;
          //printf("got curAnn\n");
	  if(curAnn != ANN_FUNCTION){
	    funcCheck = true;
	    intermediate = false;
	    caughtErrors = true;
	    //printf("not a function \n");
	  }
	  
	}
	if (funcCheck) {
	  if(type_checker){asc_error(tok_line, tok_col, "function does not exist!\n");}}
        else {
	  
	current_function = f;
	f -> isAs = isAs;
        //printf("checking domain entry\n");
	symEntry* fnDomEntry = function_get_domain(f);
	//printf("checking to see if it's my as.rec.scope->entry count change that messed things up!\n");
	    //fprintf(stdout, "%d, %d\n", fnDomEntry->type->as.rec.entryCount, $2.count);
	
	//printf("got domain entry stuff\n");
        int param_offset = 16;
        if ($2.count==1) {
            typeExpr* type = type_single(fnDomEntry);
            symEntry* paramEntry = sym_make_param($2.cur.name, type, f);
            addEntry(curTable, paramEntry);
            paramEntry->offset = param_offset;

        }
        else if(fnDomEntry->type->kind != TYPE_RECORD) {
	  intermediate = false;
	  caughtErrors = true;
	  //printf("not a record \n");
	  if(type_checker){ asc_error(tok_line, tok_col, "multiple arguments for fn with non record domain\n");}
        }
        else if(fnDomEntry->type->as.rec.entryCount == $2.count) {
            symEntry* curRecEntry = fnDomEntry->type->as.rec.scope->entry;
            struct BasicList* curParam = &$2;
            while (curRecEntry != NULL) {
                typeExpr* type = curRecEntry->type;
                symEntry* paramEntry = sym_make_param(curParam->cur.name, type, f);
                addEntry(curTable, paramEntry);
                paramEntry->offset = param_offset;
                param_offset += 8;
                curRecEntry = curRecEntry->next;
                curParam = curParam->next;
            }
        }
        else {
	  intermediate = false;
	  caughtErrors = true;
	  //printf("element count mismatch \n");
	  if(type_checker){asc_error(tok_line, tok_col, "Element count mismatch\n");}
        }
	if (intermediate) {
            IrOp fn_op = ir_op_from_symEntry(f);
            ir_emit_func_start(fn_op);
        }
        }
} sblock {
        local_offset = 0;//offsetslop: set local var offset 0 at func start
  if (type_checker){
    //typecheck_stmt($5.astStmt, asc_file, curTable);
  }
  }
;

parameter
: L_PAREN ID R_PAREN {

  //printf("parameter detected \n");
  $$.cur = $2;
$$.count = 1;
$$.next = NULL;
$$.isAs = false;
}
| AS L_PAREN idlist R_PAREN {
  //printf("parameter detected \n");
  $$ = $3;
    $$.isAs = true;
    }
;

sblock
: L_BRACE {
    scope_push();//offsetslop
    if (sblock_open_scope == true) {
	    symbolTable *newChild =  newSymbolTable(curTable, tok_line, tok_col);
	    curTable = newChild;
    }
    else {
        sblock_open_scope = true;
    }
}
dblock_opt statement_list R_BRACE {
           symbolTable *localScope = curTable;
	   $$.astStmt = ast_new_block_stmt($4.astStmtList, localScope, localScope -> line, localScope -> col);
	    curTable = curTable->parent; 
        scope_pop();//offsetslop
        if (intermediate) {
            $$.next_list = $4.next_list;
            backpatch($4.next_list, ir_array_idx);
        }
}

//| L_BRACE dblock_opt statement_list error { asc_error(tok_line, tok_col, "missing '}'"); }
// Removed the line above since it causes shift reduce error once i added scoping stuff
//TODO: fix the brace error
;

simple_statement
: assignable assignOp expression {
    bool canTC = true;

    if (&$1 == NULL) {
        asc_error(tok_line, tok_col, "invalid assignable");
        canTC = false;
        intermediate = false;
        caughtErrors = true;
    }

    if (canTC) {
        symEntry *x_entry = NULL;

        switch ($1.type) {
        case ASS_ID:
            x_entry = getEntryInSymbolTable(curTable, $1.val.id, true);
            break;
        case ARRAY_ACCESS:
            break;
        case RECORD_ACCESS:
            break;
        default:
            asc_error(tok_line, tok_col, "invalid assignable");
            intermediate = false;
            caughtErrors = true;
            break;
        }

        Basic *bcopy = malloc(sizeof(Basic));
        *bcopy = $1.basic;

        bool isReturn = false;
        char *curName = bcopy->name;
        char *funName = current_function->name;

        if (strcmp(curName, funName) == 0) {
            isReturn = true;
        }

        if (isReturn) {
            if (current_function) {
                symEntry *funRange = function_get_range(current_function);
                symEntry *eTyping = typecheck_expr($3.astExpr, asc_file, curTable, type_checker);

                if (!eTyping) {
                    intermediate = false;
                    caughtErrors = true;
                } else {
                    if (get_type(eTyping) != get_type(funRange)) {
                        intermediate = false;
                        caughtErrors = true;

                        if (type_checker) {
                            asc_error(
                                bcopy->line,
                                bcopy->col,
                                "return statement does not match function range typing\n"
                            );
                        }
                    }
                }
            }
        } else if ($1.type != ARRAY_ACCESS && $1.type != RECORD_ACCESS) {
            $$.astStmt = ast_new_assign_stmt(&($1), $3.astExpr, $1.line, $1.col);

            bool tCheck = typecheck_stmt($$.astStmt, asc_file, curTable, type_checker);

            if (!tCheck) {
                intermediate = false;
                caughtErrors = true;
            }
        }

        if (intermediate) {

            /* IR-gen for array writes */
            /* Isn't handled by isReturn */
            if ($1.type == ARRAY_ACCESS) {

                char *arr_name = $1.val.arr_access.arr_name;
                symEntry *arr_entry = getEntryInSymbolTable(curTable, arr_name, true);
                symEntry *idx_entry = $1.val.arr_access.idx.temp_var;
                symEntry *elem_type = (get_type(arr_entry))->as.arr.elem;
                symEntry *tmp_entry = $3.temp_var;

                IrOp ir_arr = ir_op_from_symEntry(arr_entry);

                ir_array_bounds_check(arr_entry, idx_entry);

                IrOp ir_byte_idx = ir_array_preprocessing(idx_entry, elem_type);
                IrOp ir_tmp = ir_op_from_symEntry(tmp_entry);

                ir_emit_array_write(ir_arr, ir_byte_idx, ir_tmp);

                $$.next_list = NULL;

            /* IR-gen for record writes */
            /* Isn't handled by isReturn */
            } else if ($1.type == RECORD_ACCESS) {

                if (debugflag) {
                    printf("$1.type is record_access \n");
                }

                symEntry *int_entry = getEntryInSymbolTable(curTable, "integer", true);

                char *rec_name = $1.val.rec_access.rec_name;
                char *member_name = $1.val.rec_access.member_name;

                symEntry *rec = getEntryInSymbolTable(curTable, rec_name, true);
                symbolTable *rec_table = (get_type(rec))->as.rec.scope;
                symEntry *member = getEntryInSymbolTable(rec_table, member_name, false);

                symEntry *tmp_var = $3.temp_var;

                IrOp ir_rec = ir_op_from_symEntry(rec);
                IrOp ir_offset = ir_op_from_int(abs(member->offset));
                IrOp x = ir_op_from_symEntry(new_tmp_var(int_entry, false));

                ir_emit_assign(x, ir_offset);

                IrOp ir_tmp = ir_op_from_symEntry(tmp_var);

                ir_emit_array_write(ir_rec, x, ir_tmp);

                $$.next_list = NULL;

            } else if (isReturn) {

                $$.next_list = NULL;

                switch ($3.expr_type) {
               

                default: {
                    IrOp retval = ir_op_from_symEntry($3.temp_var);
                    ir_emit_return(retval);
                    break;
                }
                }

                $$.next_list = NULL;

            } else {

                IrOp x = ir_op_from_symEntry(x_entry);
                IrOp y;

                switch ($3.expr_type) {
               
                default: {
                    symEntry *y_entry = $3.temp_var;
                    y = ir_op_from_symEntry(y_entry);

                    typeExpr *yType = get_type(y_entry);
                    typeExpr *xType = get_type(x_entry);

                    typeKind xKind = xType->kind;
                    typeKind yKind = yType->kind;

                    bool lhs_is_mapping_var =
                        (xKind == TYPE_MAPPING) && x_entry->isFuncPtr;

                    bool rhs_is_static_function =
                        (yKind == TYPE_MAPPING) && y_entry->ann == ANN_FUNCTION;

                    if (lhs_is_mapping_var && rhs_is_static_function) {
                        ir_emit_func_addr_assign(x, y);  // f := foo
                    } else {
                        ir_emit_assign(x, y);            // f := g, or normal assignment
                    }

                    break;
                }
                }

                $$.next_list = NULL;
            }
        }
    }
}
| RETURN expression {
        //Add type checking here for making sure the expression matches return val of func type it's a part of
        if (debugflag) { fprintf(stderr, "typechecking return expression \n"); }
	if(current_function){
	symEntry *curRange = function_get_range(current_function);
	if (debugflag) { fprintf(stderr, "curRange of function: %p \n", curRange); }
	$$.astStmt = ast_new_return_stmt($2.astExpr,curRange,$2.line, $2.col);
         
	bool tCheck = typecheck_stmt($$.astStmt,asc_file,curTable,type_checker);
	     if (!tCheck){
	       intermediate = false;
	       caughtErrors = true;
	       if (debugflag) { printf("type check failed on  return\n"); }
	     }
	
  
        $$.next_list = NULL; //need to set this as null since if you dont it kills you (segfault)


	
	    if (intermediate) {
	      if (debugflag) { printf("we are in the intermediate code for returning expressions, expr_type = %d \n", $2.expr_type); }

	      switch ($2.expr_type) {
            case EXPR_BOOL:
	      /*
                char* temp_name = new_temp_var_name();
                symEntry* bool_entry = getEntryInSymbolTable(curTable, "Boolean", true);
                symEntry *entry = sym_make_local(temp_name, type_single(bool_entry));
                addEntry(curTable, entry);
                IrOp x = ir_op_from_symEntry(entry);
                IrOp y;

                backpatch($2.true_list, ir_array_idx);
                y = ir_op_from_bool(true);
                ir_emit_assign(x, y);
                ir_emit_goto(ir_op_from_ir_array_entry(ir_array_idx+2));
                backpatch($2.false_list, ir_array_idx);
                y = ir_op_from_bool(false);
                ir_emit_assign(x, y);
                ir_emit_return(x);
                break; */
	     case EXPR_ADDR:
	     case EXPR_CHAR:
	     case EXPR_INT:
	        symEntry *retEntry = $2.temp_var;
	        if(retEntry -> ann != ANN_FUNCTION){
	        IrOp retval = ir_op_from_symEntry($2.temp_var);
                ir_emit_return(retval);
	        }
		else{
		  //have to create a new temp var, set it to the type of retEntry, and then do the func_addr_assign, and then return that instead
		   char* temp_name = new_temp_var_name();
		   symEntry *entry = sym_make_local(temp_name, get_type(retEntry));
		   IrOp x = ir_op_from_symEntry(entry);
		   IrOp y = ir_op_from_symEntry(retEntry);
		   ir_emit_func_addr_assign(x, y);
		   ir_emit_return(x);
		}
                break;
	    
            }
            $$.next_list = NULL;
	    } }
}

| assignable EQUAL_TO expression { caughtErrors = true; intermediate = false; asc_error(tok_line, tok_col, "'=' used as assignment; did you mean ':='?"); }
;
if_first
: IF L_PAREN expression R_PAREN
{
    $$.expr = $3.astExpr;
    $$.line = $3.line;
    $$.col = $3.col;

    IrOp booleanOp = ir_op_from_symEntry($3.temp_var);

    $$.true_list = newBPList(ir_array_idx);
    ir_emit_if_true_bp(booleanOp);

    $$.false_list = newBPList(ir_array_idx);
    ir_emit_goto_bp();
};
if_initial
: 
if_first <m_instr>{ $$ = ir_array_idx; }
{
  $$.expr = $1.expr;
  $$.line = $1.line;
  $$.col = $1.col;
  $$.true_list = $1.true_list;
  $$.false_list = $1.false_list;
  $$.mInstr1 = $2;
  $$.mInstr1 = $2;
}
;
if_then
: if_initial THEN{
  $$ = $1;
}
|if_initial error{
     //asc_error($1.line, $1.col, "missing ELSE clause in IF statement");
      intermediate = false;
      yyerrok;
      caughtErrors = true;
      //printf("if_initial error\n");
}

if_prefix
: if_then sblock <n_next_list>{
        $$ = newBPList(ir_array_idx);
        if (intermediate) {
            ir_emit_goto_bp();
        }
}  
  {
        $$ = $1;
	$$.sblock1 = $2;
        $$.nextList = $3;
  }
;

if_stmt
:if_prefix ELSE <m_instr>{$$ = ir_array_idx;} sblock {
        //$3 is B, $4 is m1_instr, $7 is S1, $8 is n_next_list, $10 is m2_instr, $11 is S2
        //type check expr is bool
        //
    if(!caughtErrors){
  $$.astStmt = ast_new_if_stmt($1.expr, $1.sblock1.astStmt, $4.astStmt, $1.line, $1.col);

  bool tCheck = typecheck_stmt($$.astStmt,asc_file,curTable,type_checker);
      if(!tCheck) {
	intermediate = false;
      }
 
    }
 if (intermediate) {
            backpatch($1.true_list, $1.mInstr1);
            backpatch($1.false_list, $3);
            BPList* temp = mergeBPLists($1.sblock1.next_list, $1.nextList);
            $$.next_list = mergeBPLists(temp, $4.next_list);
        }
}
| if_prefix error  {
  //      asc_error($1.line, $1.col, "missing ELSE clause in IF statement");
      intermediate = false;
      yyerrok;
      caughtErrors = true;
      //printf("if_prefix error \n");
      $$.astStmt = NULL;          
      $$.astStmtList = NULL;
      $$.next_list = NULL;
  }

WHILE_PREFIX
: WHILE <m_instr>{ $$ = ir_array_idx; }
  L_PAREN expression R_PAREN
  {
        $$.expr = $4.astExpr;
        $$.line = $1.line;
        $$.col = $1.col;
        $$.mInstr1 = $2;

        symEntry *boolean_entry = getEntryInSymbolTable(curTable,
                                                        "Boolean",
                                                        true);
        symEntry *curEntry = typecheck_expr($4.astExpr, asc_file, curTable, type_checker);

        if (!curEntry) {
            intermediate = false;
            caughtErrors = true;
        } else {
            $4.type_info = get_type(curEntry);
	    
            if (!is_while_stmt_type_safe(&($4), boolean_entry)) {
                asc_tc_error($1.line, $1.col, "\"while\" expects type Boolean to "
                                              "immediately follow it");
                intermediate = false;
                caughtErrors = true;
            }
        }

        if (intermediate) {
	    IrOp booleanOp = ir_op_from_symEntry($4.temp_var);

            $$.true_list = newBPList(ir_array_idx);
            ir_emit_if_true_bp(booleanOp);

            $$.false_list = newBPList(ir_array_idx);
            ir_emit_goto_bp();
        } else {
            $$.true_list = NULL;
            $$.false_list = NULL;
        }

        $$.sblock1.astStmt = NULL;
        $$.nextList = NULL;
  }
;
compound_statement
: WHILE_PREFIX <m_instr>{ $$ = ir_array_idx; }
  sblock
{
        /*
          $1 = WHILE_PREFIX
          $2 = body-start marker
          $3 = midrule action
          $4 = sblock
        */

        if (intermediate) {
            // If condition is true, enter body.
            backpatch($1.true_list, $2);

            // Any dangling body next jumps go back to condition.
            backpatch($3.next_list, $1.mInstr1);

            // Normal body fallthrough also goes back to condition.
            ir_emit_goto(ir_op_from_ir_array_entry($1.mInstr1));

            // If condition is false, exit loop.
            $$.next_list = $1.false_list;
        }
}

| if_stmt {$$ = $1;}

/* Causes shift/reduce conflicts. Commented out. */
/* | IF L_PAREN expression R_PAREN sblock ELSE sblock { asc_error(tok_line, tok_col, "missing 'then'"); } */



| sblock {$$.next_list = $1.next_list; $$.astStmt = NULL;}
//| WHILE L_PAREN expression error sblock {asc_error(tok_line, tok_col, "missing ')'"); }
//| IF L_PAREN expression error THEN sblock ELSE sblock { asc_error(tok_line, tok_col, "missing ')'"); }
//Theres gotta be a better way to do this, i have to comment these out like the 'missing }'
//since it causes shift/reduce conflicts
;

statement_list
: compound_statement <m_instr>{$$ = ir_array_idx;} statement_list {
        //$1 is L, $2 is m_instr, $3 is statement
  if ($1.astStmt != NULL) {
  if (type_checker){
    //  typecheck_stmt($1.astStmt,asc_file,curTable);
 }
  }
         if ($1.astStmt == NULL) { //only doing if $$ == null because I set while to be null for now
          $$.astStmtList = $3.astStmtList;
      } else {
	   $$.astStmtList = ast_new_stmt_list($1.astStmt);
          $$.astStmtList->next = $3.astStmtList;
      }
        if (intermediate) {
            backpatch($1.next_list, $2);
            $$.next_list = $3.next_list;
        }
}
| compound_statement {
  if ($1.astStmt != NULL) {
  if (type_checker){
    //typecheck_stmt($1.astStmt,asc_file,curTable);
 }
  }
  if ($1.astStmt == NULL){ $$.astStmtList = NULL;}
  else{ $$.astStmtList = ast_new_stmt_list($1.astStmt); }

  $$.next_list = $1.next_list;}
| simple_statement SEMI_COLON statement_list {
  if ($1.astStmt != NULL) {
  if (type_checker){
    //typecheck_stmt($1.astStmt,asc_file,curTable);
 }
  }
  if ($1.astStmt == NULL) {
          $$.astStmtList = $3.astStmtList;
      } else {
          $$.astStmtList = ast_new_stmt_list($1.astStmt);
          $$.astStmtList->next = $3.astStmtList;
      }
        if (intermediate) {
            $$.next_list = mergeBPLists($1.next_list, $3.next_list);
        }
}
| simple_statement SEMI_COLON {
  if ($1.astStmt != NULL) {
  if (type_checker){
    //typecheck_stmt($1.astStmt,asc_file,curTable);
 }
  }
  if ($1.astStmt == NULL){}
  else{ $$.astStmtList = ast_new_stmt_list($1.astStmt);}
  $$.next_list = $1.next_list;}
;




%%
void yyerror(char const* s) {
    asc_error(tok_line, tok_col, s);
}

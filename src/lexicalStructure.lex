%option noyywrap
%option stack
%option yylineno
%{
#include "../build/grammar.tab.h"
#include <stdio.h>
#include <stdbool.h>

extern FILE *asc_file;

extern bool debugflag;

static char line_buf[4096];
static int  line_buf_pos = 0;

static void flush_line(int lineno) {
    if (asc_file) {
        line_buf[line_buf_pos] = '\0';
        fprintf(asc_file, "%03d: %s\n", lineno, line_buf);
    }
    line_buf_pos = 0;
}

#define YY_USER_ACTION                                      \
    for (int _i = 0; _i < yyleng; _i++) {                  \
        if (yytext[_i] == '\n') {                           \
            flush_line(yylineno - 1);                       \
            yycolumn = 1;                                   \
        } else {                                            \
            if (line_buf_pos < (int)sizeof(line_buf) - 1)  \
                line_buf[line_buf_pos++] = yytext[_i];      \
        }                                                   \
    }

  /* to create track of the column count */
int yycolumn = 1;
int yy_return_comment = 0;
  /* yyleng is the length of the current token, in documentation */
static void advance_column(void) { yycolumn += yyleng; }

  /* since yyleng increases by the length of the token, if a token is in the middle of being built it keeps adding the length so far which screws up strings (and maybe comments)*/
//static void advance_column_amt(int) { yycolumn += amt; }


int tok_line = 1;
int tok_col  = 1;


static void mark_token_line_column(void) { tok_line = yylineno; tok_col = yycolumn; }

%}
%x str
%x esc
ID  [A-Za-z_][A-Za-z_0-9]*

%x COMMENTS
TAB      [\t]+
WS       [\r]+
NL      \n 
%%

\"([^\"\n\\]|\\\"|\\\\|\\n|\\t|\\a|\\b|\\r|\\f|\\v)*\" {
  if (debugflag) { printf("encountered string literal in lexer \n"); }
  mark_token_line_column();
   advance_column();

   yylval.k.type = STRING;
   yylval.k.typeName = "string";
   yylval.k.line = tok_line;
   yylval.k.col  = tok_col;

 
   char *s = strdup(yytext + 1);  
   s[yyleng - 2] = '\0';          
   yylval.k.value.s = s;
   
   if (debugflag) { printf("encountered string literal %s \n",s); }
   return C_STRING;
}
"("         {
                mark_token_line_column();
                advance_column();
                /* yylval.basic.name = strdup(yytext); */
                yylval.basic.line = tok_line;
                yylval.basic.col = tok_col;
                return L_PAREN;
            }

")" {mark_token_line_column(); advance_column(); return R_PAREN; }
"[" {mark_token_line_column(); advance_column(); return L_BRACKET; }
"]" {mark_token_line_column(); advance_column(); return R_BRACKET; }
"{" {mark_token_line_column(); advance_column(); return L_BRACE; }
"}" {mark_token_line_column(); advance_column(); return R_BRACE; }

";" {mark_token_line_column(); advance_column(); return SEMI_COLON; }
":" {mark_token_line_column(); advance_column();  yylval.basic.name = strdup(yytext);
                yylval.basic.line = tok_line;
                yylval.basic.col = tok_col;return COLON; }
"," {mark_token_line_column(); advance_column(); return COMMA; }
"->" { mark_token_line_column(); advance_column(); return ARROW; }

integer     {
                mark_token_line_column();
                advance_column();
                yylval.basic.name = strdup(yytext);
                yylval.basic.line = tok_line;
                yylval.basic.col = tok_col;
		return T_INTEGER;
            }

address     {
                mark_token_line_column();
                advance_column();
                yylval.basic.name = strdup(yytext);
                yylval.basic.line = tok_line;
                yylval.basic.col = tok_col;
		return T_ADDRESS;
            }

Boolean     {
                mark_token_line_column();
                advance_column();
                yylval.basic.name = strdup(yytext);
                yylval.basic.line = tok_line;
                yylval.basic.col = tok_col;
		return T_BOOLEAN;
            }

character   {
                mark_token_line_column();
                advance_column();
                yylval.basic.name = strdup(yytext);
                yylval.basic.line = tok_line;
                yylval.basic.col = tok_col;
		return T_CHARACTER;
            }

string      {
                mark_token_line_column();
                advance_column();
                yylval.basic.name = strdup(yytext);
                yylval.basic.line = tok_line;
                yylval.basic.col = tok_col;
		return T_STRING;
            }

true        {
                mark_token_line_column();
                advance_column();
		yylval.k.type = BOOLEAN;
		yylval.k.typeName = "Boolean";
		yylval.k.line = tok_line;
		yylval.k.col  = tok_col;
		yylval.k.value.b = true;
		return C_TRUE;
            }

false       {
                mark_token_line_column();
                advance_column();
		yylval.k.type = BOOLEAN;
                yylval.k.typeName = "Boolean";
                yylval.k.line = tok_line;
                yylval.k.col  = tok_col;
                yylval.k.value.b = false; // <WHY DID THIS SAY TRUE BEFORE? LMAO
		return C_FALSE;
            }

null        {
                mark_token_line_column();
                advance_column();
		return C_NULL;
            }

while       {
                mark_token_line_column();
                advance_column();
                /* yylval.basic.name = strdup(yytext); */
                yylval.basic.line = tok_line;
                yylval.basic.col = tok_col;
		return WHILE;
            }

if          {
                mark_token_line_column();
                advance_column();
		return IF;
            }

then        {
                mark_token_line_column();
                advance_column();
		return THEN;
            }

else        {
                mark_token_line_column();
                advance_column();
		return ELSE;
            }

type        {
                mark_token_line_column();
                advance_column();
		return TYPE;
            }

function    {
                mark_token_line_column();
                advance_column();
		return FUNCTION;
            }

return      {
                mark_token_line_column();
                advance_column();
		return RETURN;
            }

external    {
                mark_token_line_column();
                advance_column();
		return EXTERNAL;
            }

as          {
                mark_token_line_column();
		advance_column();
                return AS;
            }

reserve     { 
                mark_token_line_column();
                advance_column();
                yylval.basic.name = strdup(yytext);
                yylval.basic.line = tok_line;
                yylval.basic.col = tok_col;
                return RESERVE;
            }

release     { 
                mark_token_line_column();
                advance_column();
                yylval.basic.name = strdup(yytext);
                yylval.basic.line = tok_line;
                yylval.basic.col = tok_col;
                return RELEASE;
            }

{ID}        {
               mark_token_line_column();
	       advance_column();
           yylval.basic.name = strdup(yytext);
           yylval.basic.line = tok_line;
           yylval.basic.col = tok_col;
               return ID;
            }



":="  { mark_token_line_column(); advance_column(); return ASSIGN; }
"+"   { mark_token_line_column(); advance_column(); return ADD; }
"-"   { mark_token_line_column(); advance_column(); return SUB_OR_NEG; }
"*"   { mark_token_line_column(); advance_column(); return MUL; }
"/"   { mark_token_line_column(); advance_column(); return DIV; }
"%"   { mark_token_line_column(); advance_column(); return REM; }
"<"   { mark_token_line_column(); advance_column(); return LESS_THAN; }
"="   { mark_token_line_column(); advance_column(); return EQUAL_TO; }
"!"   { mark_token_line_column(); advance_column(); return NOT; }
"&"   { mark_token_line_column(); advance_column(); return AND; }
"|"   { mark_token_line_column(); advance_column(); return OR; }

"."     {
            mark_token_line_column();
            advance_column();
            /* yylval.basic.name = strdup(yytext); */
            yylval.basic.line = tok_line;
            yylval.basic.col = tok_col;
            return DOT; 
        }


[0-9]+ {
    mark_token_line_column();
    advance_column();

    yylval.k.type = INTEGER;
    yylval.k.typeName = "integer";      
    yylval.k.line = tok_line;
    yylval.k.col  = tok_col;
    yylval.k.value.i = atoi(yytext);  

    return C_INTEGER; 
}


'([^'\\\n]|\\n|\\t|\\'|\\\\)' {
    mark_token_line_column();
    advance_column();

    char c;

    if (yytext[1] == '\\') {
        switch (yytext[2]) {
            case 'n':
                c = '\n';
                break;
            case 't':
                c = '\t';
                break;
            case '\'':
                c = '\'';
                break;
            case '\\':
                c = '\\';
                break;
            default:
                if (debugflag) { fprintf(stderr, "Unknown escape sequence: %s\n", yytext); }
                c = yytext[2]; // fallback
                break;
        }
    } else {
        c = yytext[1];
    }

    yylval.k.type = CHARACTER;
    yylval.k.typeName = "character";
    yylval.k.line = tok_line;
    yylval.k.col  = tok_col;
    yylval.k.value.c = c;

    if (debugflag) { fprintf(stderr, "encountered character literal : %c \n", yylval.k.value.c); }
    return C_CHARACTER;
}

{NL} {yycolumn = 1;}
{TAB} {advance_column();}
{WS}  {advance_column();}


\(\* {
    mark_token_line_column();
    yycolumn += yyleng;
    yy_push_state(COMMENTS);
}

<COMMENTS>\*\) {
    yycolumn += yyleng;
    yy_pop_state();
}

<COMMENTS>\n {
  //yylineno++;
    yycolumn = 1;
}

<COMMENTS>[^*\n]+ {
    yycolumn += yyleng;
}

<COMMENTS>\* {
    yycolumn += yyleng;
}

<COMMENTS><<EOF>> {
    return COMMENT_EOF;
}

<<EOF>> {if (line_buf_pos > 0) {
        flush_line(yylineno);
    }
  return 0; }

. {   
  advance_column();          
    
}

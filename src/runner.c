#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>
#include <stdlib.h>
#include "../src/symbol-table.h"
#include "../src/tokens.h"

#include "../src/ir.h"
#include "../src/backend.h"

extern int yylex();
extern int yyparse();
extern FILE* yyin;

extern symbolTable* headTable;

extern char *yytext;
extern int tok_line;
extern int tok_col;
extern int yy_return_comment;

bool output_tokens = false;
bool output_symbol_table = false;
bool output_annotated_source = false;
bool type_checker = false;
bool intermediate = false;
FILE *asc_file = NULL;
bool assembly = false;
bool debug = false;
bool debugflag = false;
bool caughtErrors = false;

void print_help(void);
char* isolate_filename_from_path(char *path) {
    char* filename = strrchr(path, '/');
    if (filename) {
        filename++;
    }
    else {
        filename = path;
    }
    char* extension = strrchr(path, '.');
    if (extension) {
        *extension = '\0';
    }
    return filename;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("No input provided\n");
        return 0;
    }
    FILE* in_file = NULL;
    char* in_file_name = NULL;
    char* filename;

    //Read arguments
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-help") == 0) {
                print_help();
                return 0;
            }
            else if (strcmp(argv[i], "-tok") == 0) {
                output_tokens = true;
		        yy_return_comment = 1;
            }
            else if (strcmp(argv[i], "-st") == 0) {
                output_symbol_table = true;
            }
            else if (strcmp(argv[i], "-asc") == 0) {
                output_annotated_source = true;
            }
            else if (strcmp(argv[i], "-tc") == 0) {
                type_checker = true;
            }
            else if (strcmp(argv[i], "-ir") == 0) {
                intermediate = true;
            }
            else if (strcmp(argv[i], "-cg") == 0) {
                assembly = true;
            }
            else if (strcmp(argv[i], "-debug") == 0) {
                debugflag = true;
                debug = true;
            }
            continue;
        }

        in_file_name = strdup(argv[i]);
        in_file = fopen(in_file_name, "r");
        
        filename = isolate_filename_from_path(strdup(argv[i]));
    }

    

    if (in_file == NULL) {
        printf("No input file provided!\n");
        return 1;
    }

    yyin = in_file;

    newSymbolTable(NULL, 1, 1);
    initializeGlobalSymbols();

    if (output_annotated_source) {
      char asc_name[256];
      strncpy(asc_name, filename, sizeof(asc_name) - 5);
      strcat(asc_name, ".asc");
      asc_file = fopen(asc_name, "w");
    }

    int syntaxErrors = yyparse();
    if (asc_file) {
        fclose(asc_file);
    }

    if (syntaxErrors || caughtErrors) {//yyparse returning 1 means syntax error, caughtErrors handles typechecking/other errors
        puts("Program could not compile due to errors\n");
	    return 1;
    }


    if (output_symbol_table) {
        FILE* st_file; 
        char st_name[256];
        strncpy(st_name, filename, sizeof(st_name) - 4);
        strcat(st_name, ".st");
        st_file = fopen(st_name, "w");
        print_symbol_table(headTable, st_file);
        fclose(st_file);
    }

    if (intermediate) {
        FILE* ir_file; 
        char ir_name[256];
        strncpy(ir_name, filename, sizeof(ir_name) - 4);
        strcat(ir_name, ".ir");
        ir_file = fopen(ir_name, "w");
        ir_array_print(ir_file);
        fclose(ir_file);

        //Codegen should only happen if intermediate executes fully without issue

        if (assembly) {
            FILE* cg_file; 
            char cg_name[256];
            strncpy(cg_name, filename, sizeof(cg_name) - 3);
            strcat(cg_name, ".s");
            cg_file = fopen(cg_name, "w");
            codegen(cg_file);
            fclose(cg_file);
        }
    }
    if (output_tokens) {
      // rewind before lexing
      fclose(yyin);
      in_file = fopen(in_file_name, "r");
      yyin = in_file;

      bool quit = false;
      int next_token;

      FILE* FILE_tok = NULL;
      char t_name[256];
      strncpy(t_name, filename, sizeof(t_name) - 5);
      strcat(t_name, ".tok");
      FILE_tok = fopen(t_name, "w");

      yy_return_comment = 1;

      while (!quit) {
        next_token = yylex();
        if (next_token == 0) {
	  quit = true;
	  break;
        }
        fprintf(FILE_tok, "%d %d %3d \"%s\"\n",
                tok_line, tok_col, next_token, yytext);
      }

      fclose(FILE_tok);

      
      yy_return_comment = 0;
    }
    return 0;
}

void print_help(void) {
    printf("HELP:\n");
    printf("How to run the alpha compiler:\n");
    printf("./alpha [options] program\n");
    printf("Valid options:\n");
    printf("-tok   output token info to .tok file\n");
    printf("-st    output symbol table to .st file\n");
    printf("-asc   output annotated source to .asc file\n");
    printf("-tc    run type checker (errors in .asc)\n");
    printf("-ir    generate intermediate representation (.ir)\n");
    printf("-cg    generate assembly (.s)\n");
    printf("-debug print debug messages\n");
    printf("-help  print this message and exit\n");
}


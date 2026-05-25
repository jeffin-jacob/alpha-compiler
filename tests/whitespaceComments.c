#include "../src/tokens.h"
#include "stdio.h"

extern int yylex();
extern FILE* yyin;
extern int tok_line;
extern int tok_col;
extern int yycolumn;
extern int yylineno;
extern void yyrestart(FILE *input_file);
void refreshScanner(FILE *input){
  yyrestart(input);
  tok_line = 1;
  tok_col = 1;
  yycolumn = 1;
  yylineno = 1;
}
int test_comment1() {
    FILE *f = fopen("tests/data/comment1.txt", "r");
    if (f==NULL) {
        printf("FILE DOESNT EXIST\n");
        return 1;
    }
    yyin = f;
    refreshScanner(yyin);
    int t = yylex();
    if (t != COMMENT) {
        printf("EXPECTED COMMENT(700) GOT: %i\n", t);
	fclose(yyin);
	return 1;
    }
    
    if (tok_line != 5 || tok_col != 3) {
	printf("EXPECTED tok_line 5 AND tok_column = 3, GOT: tok_line %i , tok_column %i \n", tok_line, tok_col);
	fclose(yyin);
	return 1;
      }
    fclose(yyin);
    return 0;
}
int test_comment2() {
    FILE *f = fopen("tests/data/comment2.txt", "r");
    if (f==NULL) {
        printf("FILE DOESNT EXIST\n");
        return 1;
    }
    yyin = f;
    refreshScanner(yyin);
    int t = yylex();
    if (t != COMMENT) {
        printf("EXPECTED COMMENT(700) GOT: %i\n", t);
	fclose(yyin);
	return 1;
    }

    if (tok_line != 1 || tok_col != 1) {
        printf("EXPECTED tok_line 1 AND tok_column = 1, GOT: tok_line %i , tok_column %i \n", tok_line, tok_col);
	fclose(yyin);
	return 1;
      }

    t = yylex();
    if (t != COMMENT) {
        printf("EXPECTED SECOND COMMENT(700) GOT: %i\n", t);
	fclose(yyin);
	return 1;
    }
    if (tok_line != 6 || tok_col != 12) {
        printf("EXPECTED tok_line 6 AND tok_column = 12, GOT: tok_line %i , tok_column %i \n", tok_line, tok_col);
        fclose(yyin);
        return 1;
      }
    fclose(yyin);
    return 0;
}
int test_comment3() {
    FILE *f = fopen("tests/data/comment3.txt", "r");
    if (f==NULL) {
        printf("FILE DOESNT EXIST\n");
	return 1;
    }
    yyin = f;
    refreshScanner(yyin);
    int t = yylex();
    if (t != COMMENT_EOF) {
        printf("EXPECTED COMMENT_EOF ERROR (702) GOT: %i\n", t);
	fclose(yyin);
	return 1;
    }

    fclose(yyin);
    return 0;
}
int test_comment4() {
    FILE *f = fopen("tests/data/comment4.txt", "r");
    if (f==NULL) {
        printf("FILE DOESNT EXIST\n");
        return 1;
    }
    yyin = f;
    refreshScanner(yyin);
    int t = yylex();
    t = yylex();
    t = yylex();
    if (t != ID){
      printf("EXPECTED ID (101) GOT: %i\n", t);
    }
    if (tok_line != 7 || tok_col != 40){
       printf("EXPECTED tok_line 7 AND tok_column = 40, GOT: tok_line %i , tok_column %i \n", tok_line, tok_col);
       return 1;
    }

    fclose(yyin);
    return 0;
}
int main(){
  printf("testing detection and placement of a single comment\n");
  if(test_comment1()){
    printf("FAILED\n");
  }
  else{
    printf("PASSED \n");
  }
  printf("testing detection and placement of two comments\n");
  if(test_comment2()){
    printf("FAILED\n");
  }
  else{
    printf("PASSED \n");
  }
   printf("testing my own comment EOF thing\n");
  if(test_comment3()){
    printf("FAILED\n");
  }
  else{
    printf("PASSED \n");
  }
  printf("testing if line and column count are correct after a multi line comment!\n");
  if(test_comment4()){
    printf("FAILED\n");
  }
  else{
    printf("PASSED \n");
  }
  return 0;
}

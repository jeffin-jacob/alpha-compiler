#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_FILE "tests/data/input3.txt"
#define OUTPUT_FILE "tests/data/output3.txt"

#define MAX_LENGTH 1000

char *test_name;

extern FILE *yyin;
extern char *yytext;
extern int tok_line;
extern int tok_col;
int yylex();

static int diff(FILE *in, FILE *out) {
    int token;
    char line[MAX_LENGTH];
    int curr_line = 1;
    int errors = 0;
    while ((token = yylex()) != 0 && fgets(line, MAX_LENGTH, out)) {
        char actual_line[MAX_LENGTH];
        sprintf(actual_line, "%d %s %d %d\n", token, yytext, tok_line, tok_col);
        if (strcmp(actual_line, line) != 0) {
            actual_line[strlen(actual_line) - 1] = '\0';
            line[strlen(line) - 1] = '\0';
            printf("%s: Line %d: \"%s\" <> \"%s\"\n", test_name, curr_line,
                   actual_line, line);
            errors++;
        }
        curr_line++;
    }
    return errors;
}

int main(int argc, char *argv[]) {
    yyin = fopen(INPUT_FILE, "r");
    if (yyin == NULL) {
        fprintf(stderr, "%s: Unable to read\n", INPUT_FILE);
        return 1;
    }
    FILE *out;
    out = fopen(OUTPUT_FILE, "r");
    if (out == NULL) {
        fprintf(stderr, "%s: Unable to read\n", OUTPUT_FILE);
        fclose(yyin);
        return 1;
    }
    test_name = argv[0];
    int errors;
    if ((errors = diff(yyin, out))) {
        printf("%s: %d mismatch(es). Test Failed\n", test_name, errors);
        fclose(yyin);
        fclose(out);
        return 1;
    }
    printf("%s: Test Passed\n", test_name);
    fclose(yyin);
    fclose(out);
    return 0;
}

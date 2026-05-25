CC := gcc
#CFLAGS := -Wall -g
CFLAGS := -O0
LDFLAGS := -lfl
OUTNAME := alpha

LEX := flex
YACC := bison

all: compiler

build:
	mkdir -p build

bin:
	mkdir -p bin



#lexer and parser targets

build/grammar.tab.c: src/grammar.y | build
	$(YACC) --header $< -o $@
	
build/lex.yy.c: src/lexicalStructure.lex build/grammar.tab.c | build
	$(LEX) $(LFLAGS) -o $@ $<

build/lex.yy.o: build/lex.yy.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/grammar.tab.o: build/grammar.tab.c | build
	$(CC) $(CFLAGS) -c -o $@ $<


#symbol table targets
build/symEntry.o: src/symEntry.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/symbol-table.o: src/symbol-table.c src/symbol-table.h src/symEntry.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

#ir targets
build/ir.o: src/ir.c src/ir.h src/symbol-table.h src/symEntry.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/ast.o: src/ast.c src/ast.h src/symbol-table.h src/symEntry.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

#backend targets
build/backend.o: src/backend.c src/ir.h src/symbol-table.h src/symEntry.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

# compiler targets

build/runner.o: src/runner.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

compiler: build/runner.o build/lex.yy.o build/grammar.tab.o build/symEntry.o build/symbol-table.o build/ir.o build/ast.o build/backend.o | bin
	$(CC) -o bin/$(OUTNAME) $^


#test targets
build/test_cg_structs.o: tests/test_cg_structs.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

test_cg_structs: build/test_cg_structs.o build/symEntry.o build/symbol-table.o build/ir.o build/ast.o build/backend.o | bin
	$(CC) $(CFLAGS) -o bin/test_cg_structs $^ $(LDFLAGS)

test_ir_print: build/ir.o | bin
	$(CC) $(CFLAGS) tests/test_ir_print.c -o bin/test_ir_print $^ $(LDFLAGS)
	./bin/test_ir_print

build/test_ir_structure.o: tests/test_ir_structure.c src/ir.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

bin/test_ir_structure: build/test_ir_structure.o | bin
	$(CC) $(CFLAGS) -o $@ $^
  
build/whitespaceComments.o: tests/whitespaceComments.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

whitespaceComments: build/lex.yy.o build/whitespaceComments.o | bin
	$(CC) $(CFLAGS) -o bin/whitespaceComments $^ $(LDFLAGS)
	./bin/whitespaceComments


build/testSymEntry.o: tests/testSymEntry.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

testSymEntry: build/testSymEntry.o build/symEntry.o | bin
	$(CC) $(CFLAGS) -o bin/testSymEntry $^ $(LDFLAGS)
	./bin/testSymEntry

build/test_string_punc.o: tests/test_string_punc.c | build
	$(CC) -c -o $@ $<

test_string_punc: build/lex.yy.o build/test_string_punc.o | bin
	$(CC) build/lex.yy.o build/test_string_punc.o -o bin/test_string_punc
	./bin/test_string_punc

bin/test3: build/test3.o build/lex.yy.o | bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	
build/test3.o: tests/test3.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

bin/test4: build/test4.o | bin
	$(CC) $(CFLAGS) -o $@ $^
	
build/test4.o: tests/test4.c src/tokens.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

bin/test17: build/test17.o build/lex.yy.o build/grammar.tab.o | bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	
build/test17.o: tests/test17.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

bin/test26: build/test26.o build/symbol-table.o build/symEntry.o build/grammar.tab.o build/lex.yy.o | bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

build/test26.o: tests/test26.c src/symbol-table.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

bin/test19: build/test19.o build/symbol-table.o build/symEntry.o | bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

build/test19.o: tests/test19.c src/symbol-table.h src/symEntry.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

bin/test27: build/test27.o build/symbol-table.o build/symEntry.o build/grammar.tab.o build/lex.yy.o | bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

build/test27.o: tests/test27.c src/symbol-table.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

bin/test29: build/runner.o build/lex.yy.o build/grammar.tab.o build/symEntry.o build/symbol-table.o build/ir.o build/ast.o | bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bin/test36: build/runner.o build/lex.yy.o build/grammar.tab.o build/symEntry.o build/symbol-table.o build/ir.o build/ast.o | bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_param_st: tests/test_param_st.c build/symbol-table.o build/symEntry.o build/grammar.tab.o build/lex.yy.o | bin
	$(CC) $(CFLAGS) -o bin/test_param_st $^ $(LDFLAGS)

build/test_expr.o: tests/test_expr.c | build
	$(CC) $(CFLAGS) -c -o $@  $<

test_expr: build/test_expr.o build/lex.yy.o build/grammar.tab.o | bin
	$(CC) $(CFLAGS) -o bin/test_expr $^ $(LDFLAGS)

test_print_st: build/test_print_st.o build/symbol-table.o build/symEntry.o | bin
	$(CC) $(CFLAGS) -o bin/test_print_st $^ $(LDFLAGS)

build/test_print_st.o: tests/test_print_st.c src/symbol-table.h src/symEntry.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/test_ir_assign.o: tests/test_ir_assign.c src/ir.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

bin/test_ir_assign: build/test_ir_assign.o build/ir.o | bin
	$(CC) $(CFLAGS) -o $@ $^

build/test_ir_arith.o: tests/test_ir_arith.c src/ir.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

bin/test_ir_arith: build/test_ir_arith.o build/ir.o | bin
	$(CC) $(CFLAGS) -o $@ $^

build/test_ir_jump.o: tests/test_ir_jump.c src/ir.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

bin/test_ir_jump: build/test_ir_jump.o build/ir.o | bin
	$(CC) $(CFLAGS) -o $@ $^



clean:
	rm -rf build bin
	-rm -f *.tok *.st *.ir *.asc *.s *.out

.PHONY: compiler test clean

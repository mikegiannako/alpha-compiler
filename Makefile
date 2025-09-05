CC = gcc
CFLAGS = -Wall -g
BFLAGS = --yacc --defines
DEBUG = -DDEBUG

.PHONY: all clean

compiler: bison tokenizer src/lex.l src/utils.c src/lexer.c src/generic_stack.c src/memory_operations.c
	$(CC) $(CFLAGS) $(DEBUG) -o build/compiler build/parser.c build/tokenizer.c src/utils.c src/lexer.c src/generic_stack.c src/memory_operations.c

bison:	src/parser.y
	bison $(BFLAGS) --output=build/parser.c src/parser.y

tokenizer: src/lex.l
	flex -o build/tokenizer.c src/lex.l

clean:
	-rm -f build/* quads.txt instructions.txt *.bin
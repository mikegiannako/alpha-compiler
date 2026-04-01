CC = gcc
CFLAGS = -Wall -g -fsanitize=address
BFLAGS = --defines
DEBUG = -DDEBUG

SRCS = \
	src/utils.c \
	src/lexer.c \
	src/generic_stack.c \
	src/memory_operations.c \
	src/symtable.c \
	src/rule_handlers.c \
	src/intermediate.c

.PHONY: all clean

compiler: bison tokenizer src/lex.l $(SRCS)
	$(CC) $(CFLAGS) $(DEBUG) -o build/compiler build/parser.c build/tokenizer.c $(SRCS) -lfl

bison:	src/parser.y
	bison $(BFLAGS) --output=build/parser.c src/parser.y

tokenizer: src/lex.l
	flex -o build/tokenizer.c src/lex.l

clean:
	-rm -f build/* quads.txt instructions.txt *.bin
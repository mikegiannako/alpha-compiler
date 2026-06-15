CC = gcc
CFLAGS = -Wall -g -fsanitize=address
BFLAGS = --defines
DEBUG = -DDEBUG

# Shared code (memory ops, error macros) lives in the repo-root common/ dir.
COMMON = ../common
INCLUDES = -I$(COMMON)

SRCS = \
	src/utils.c \
	src/lexer.c \
	src/generic_stack.c \
	$(COMMON)/memory_operations.c \
	src/symtable.c \
	src/rule_handlers.c \
	src/intermediate.c \
	src/quad.c \
	src/config.c \
	src/instruction.c \
	src/vm_code.c \
	src/generator.c \
	src/binary.c

.PHONY: all clean

compiler: bison tokenizer src/lex.l $(SRCS)
	$(CC) $(CFLAGS) $(DEBUG) $(INCLUDES) -o build/compiler build/parser.c build/tokenizer.c $(SRCS) -lfl

bison:	src/parser.y
	bison $(BFLAGS) --output=build/parser.c src/parser.y

tokenizer: src/lex.l
	flex -o build/tokenizer.c src/lex.l

clean:
	-rm -f build/* quads.txt instructions.txt *.bin
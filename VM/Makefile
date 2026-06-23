# Compiler and tools
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lm

# Directories
SRCDIR = src
HEADERDIR = include

# Shared code (memory ops, error macros) lives in the repo-root common/ dir.
COMMON = ../common
INCLUDES = -I$(COMMON)

SOURCES = $(SRCDIR)/binary_reader.c $(SRCDIR)/main.c $(SRCDIR)/avm_structs.c $(SRCDIR)/avm_dispatcher.c $(SRCDIR)/avm_trace.c $(COMMON)/memory_operations.c

# Target executable
TARGET = vm

# -DDEBUG enables verbose VM diagnostics (binary-load info + startup banner and
# the input echo). Off by default; `make debug` switches it on.
DEBUG =

# Default target
all: $(TARGET)

# Build the executable directly from source files. The FORCE prerequisite relinks
# on every invocation so switching between a normal and a -DDEBUG build never
# leaves a stale binary (the compiler build relinks every time too).
$(TARGET): $(SOURCES) FORCE
	$(CC) $(CFLAGS) $(DEBUG) $(INCLUDES) -o $(TARGET) $(SOURCES) $(LDFLAGS)

# Verbose build with -DDEBUG diagnostics enabled.
debug: DEBUG = -DDEBUG
debug: all

# Clean executable
clean:
	rm -f $(TARGET)

# Force rebuild
rebuild: clean all

FORCE:

.PHONY: all debug clean rebuild FORCE
COMPILER := Compiler/build/compiler
VM       := VM/vm
BIN      ?= output.bin
FLAGS    ?=
OUTFILE  ?= debug.out
LOGFILE  ?= debug.log

.DEFAULT_GOAL := help
.PHONY: help run debug compiler vm clean

help:
	@echo "make run   test=<path> [FLAGS=...]   build, compile and run"
	@echo "make debug test=<path>               full trace; saves $(OUTFILE) / $(LOGFILE)"
	@echo "make compiler | vm | clean"
	@echo "(.asc / .alpha extension may be omitted)"

compiler:
	$(MAKE) -C Compiler

vm:
	$(MAKE) -C VM

run: compiler vm
	./$(COMPILER) $(FLAGS) $(test) $(BIN) && ./$(VM) $(BIN)

debug:
	$(MAKE) -C Compiler debug
	$(MAKE) -C VM debug
	{ ASAN_OPTIONS=detect_leaks=0 ./$(COMPILER) $(FLAGS) $(test) $(BIN) && \
	./$(VM) --dump $(BIN); } 2>$(LOGFILE) | tee $(OUTFILE)

clean:
	$(MAKE) -C Compiler clean
	$(MAKE) -C VM clean
	-rm -f $(BIN) instructions.txt quads.txt $(OUTFILE) $(LOGFILE)

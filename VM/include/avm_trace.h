#ifndef _AVM_TRACE_H_
#define _AVM_TRACE_H_

#include <stdio.h>
#include "binary_reader.h"   // VMInstruction, VMArg

// Trace levels, set from the command line (see main.c):
//   vmTraceInstr -> print a one-line trace of each instruction before it runs
//   vmTraceStack -> additionally dump the annotated execution stack each step
extern unsigned char vmTraceInstr;
extern unsigned char vmTraceStack;

// Print a single instruction: "[pc] L<line> opcode  result  arg1  arg2".
void VMInstruction_Print(FILE* out, VMInstruction* instr, unsigned pc);

// Dump the execution stack: globals, then each call frame (env cells, formals,
// locals) walked via the saved-BP chain. Cell values are rendered briefly --
// tables are shown as a handle, not expanded -- so cyclic tables are safe.
void VMStack_Dump(FILE* out);

// Hook called from the execute cycle before each instruction; honours the flags.
void VMTrace_Step(VMInstruction* instr, unsigned pc);

#endif // _AVM_TRACE_H_

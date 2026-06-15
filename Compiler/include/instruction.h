#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

#include <stdio.h>

typedef enum Iopcode {
    ASSIGN_IOPCODE,
    ADD_IOPCODE,
    SUB_IOPCODE,
    MUL_IOPCODE,
    DIV_IOPCODE,
    MOD_IOPCODE,
    IF_EQ_IOPCODE,
    IF_NOTEQ_IOPCODE,
    IF_LESSEQ_IOPCODE,
    IF_GREATEREQ_IOPCODE,
    IF_LESS_IOPCODE,
    IF_GREATER_IOPCODE,
    CALL_IOPCODE,
    PUSHARG_IOPCODE,
    FUNCENTER_IOPCODE,
    FUNCEXIT_IOPCODE,
    NEWTABLE_IOPCODE,
    TABLEGETELEM_IOPCODE,
    TABLESETELEM_IOPCODE,
    JUMP_IOPCODE,
    NOP_IOPCODE
} Iopcode_enum;

extern char* iopnames[];

typedef enum Iargtype {
    LABEL_IARGTYPE,
    GLOBAL_IARGTYPE,
    FORMAL_IARGTYPE,
    LOCAL_IARGTYPE,
    NUMBER_IARGTYPE,
    STRING_IARGTYPE,
    BOOL_IARGTYPE,
    NIL_IARGTYPE,
    USERFUNC_IARGTYPE,
    LIBFUNC_IARGTYPE,
    RETVAL_IARGTYPE,
    NULL_IARGTYPE
} Iargtype_enum;

extern char* iargtypes[];

typedef struct Iarg {
    Iargtype_enum type;
    unsigned int val;
}* Iarg_ptr;

typedef struct Instruction {
    Iopcode_enum opcode;
    Iarg_ptr result;
    Iarg_ptr arg1;
    Iarg_ptr arg2;
    unsigned int line;
}* Instruction_ptr;

extern Instruction_ptr* instructions;
extern unsigned int currInstruction;
extern unsigned int instructionTableSize;

void emitInstruction(Instruction_ptr);
void instructions_FreeAll(void);

void printSingleInstruction(FILE* buffer, Instruction_ptr instruction, unsigned int instNo);
void printInstructions();

#endif
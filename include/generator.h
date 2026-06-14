#ifndef _GENERATOR_H_
#define _GENERATOR_H_

#include <stdio.h>
#include "quad.h"
#include "instruction.h"

void generate_GENERIC(Quad_ptr, Iopcode_enum);
void generate_RELATIONAL(Quad_ptr, Iopcode_enum);

void generate_ASSIGN(Quad_ptr);
void generate_ADD (Quad_ptr);
void generate_SUB (Quad_ptr);
void generate_MUL (Quad_ptr);
void generate_DIV (Quad_ptr);
void generate_MOD (Quad_ptr);
void generate_UMINUS (Quad_ptr);
void generate_IF_EQ (Quad_ptr);
void generate_IF_NOTEQ (Quad_ptr);
void generate_IF_LESSEQ (Quad_ptr);
void generate_IF_GREATEREQ (Quad_ptr);
void generate_IF_LESS (Quad_ptr);
void generate_IF_GREATER (Quad_ptr);
void generate_CALL (Quad_ptr);
void generate_PARAM (Quad_ptr);
void generate_RETURN (Quad_ptr);
void generate_GETRETVAL (Quad_ptr);
void generate_FUNCSTART (Quad_ptr);
void generate_FUNCEND (Quad_ptr);
void generate_NEWTABLE (Quad_ptr);
void generate_TABLEGETELEM (Quad_ptr);
void generate_TABLESETELEM (Quad_ptr);
void generate_JUMP (Quad_ptr);
void generate_NOP (Quad_ptr);

typedef void (*GeneratorFunc_ptr) (Quad_ptr);

extern GeneratorFunc_ptr generators[];

void generateInstructions (void);

#endif
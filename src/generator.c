#include "../include/generator.h"
#include "../include/intermediate.h"
#include "../include/instruction.h"
#include "../include/vm_code.h"
#include "../include/quad.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void generate_GENERIC(Quad_ptr quad, Iopcode_enum opcode){
    Instruction_ptr instruction = calloc(1, sizeof(struct Instruction));

    instruction->opcode = opcode;

    if (quad->result) {
        instruction->result = calloc(1, sizeof(struct Iarg));
        makeOperand(quad->result ,instruction->result);
    }

    if (quad->arg1) {
        instruction->arg1 = calloc(1, sizeof(struct Iarg));
        makeOperand(quad->arg1 ,instruction->arg1);
    }

    if (quad->arg2) {
        instruction->arg2 = calloc(1, sizeof(struct Iarg));
        makeOperand(quad->arg2 ,instruction->arg2);
    }

    instruction->line = quad->line;

    emitInstruction(instruction);
}

void generate_RELATIONAL(Quad_ptr quad, Iopcode_enum opcode){
    Instruction_ptr instruction = calloc(1, sizeof(struct Instruction));

    instruction->opcode = opcode;

    if(quad->arg1){
        instruction->arg1 = calloc(1, sizeof(struct Iarg));
        makeOperand(quad->arg1 ,instruction->arg1);
    }

    if(quad->arg2){
        instruction->arg2 = calloc(1, sizeof(struct Iarg));
        makeOperand(quad->arg2 ,instruction->arg2);
    }

    instruction->result = calloc(1, sizeof(struct Iarg));
    instruction->result->type = LABEL_IARGTYPE;
    // Quads and instructions are both 1-indexed with a 1:1 mapping (quad i ->
    // instruction i), so a branch target label carries over unchanged.
    instruction->result->val = quad->label;

    instruction->line = quad->line;

    emitInstruction(instruction);
}

void generate_ASSIGN(Quad_ptr quad){
    generate_GENERIC(quad, ASSIGN_IOPCODE);
}

void generate_ADD (Quad_ptr quad){
    generate_GENERIC(quad, ADD_IOPCODE);
}

void generate_SUB (Quad_ptr quad){
    generate_GENERIC(quad, SUB_IOPCODE);
}

void generate_MUL (Quad_ptr quad){
    generate_GENERIC(quad, MUL_IOPCODE);
}

void generate_DIV (Quad_ptr quad){
    generate_GENERIC(quad, DIV_IOPCODE);
}

void generate_MOD (Quad_ptr quad){
    generate_GENERIC(quad, MOD_IOPCODE);
}

void generate_UMINUS (Quad_ptr quad){
    Instruction_ptr instruction = calloc(1, sizeof(struct Instruction));

    instruction->opcode = MUL_IOPCODE;

    instruction->arg2 = calloc(1, sizeof(struct Iarg));
    makeOperand(quad->arg1, instruction->arg2);

    // Local throwaway const expr (-1); makeOperand only reads it into the const
    // table, and it is not arena-registered, so free it right after.
    Expr_ptr expr = calloc(1, sizeof(struct Expr));
    expr->type = CONST_NUM_EXPRTYPE;
    expr->numConst = -1;
    instruction->arg1 = calloc(1, sizeof(struct Iarg));
    makeOperand(expr, instruction->arg1);
    free(expr);

    instruction->result = calloc(1, sizeof(struct Iarg));
    makeOperand(quad->result, instruction->result);

    instruction->line = quad->line;

    emitInstruction(instruction);
}

void generate_IF_EQ (Quad_ptr quad){
    generate_RELATIONAL(quad, IF_EQ_IOPCODE);
}

void generate_IF_NOTEQ (Quad_ptr quad){
    generate_RELATIONAL(quad, IF_NOTEQ_IOPCODE);
}

void generate_IF_LESSEQ (Quad_ptr quad){
    generate_RELATIONAL(quad, IF_LESSEQ_IOPCODE);
}

void generate_IF_GREATEREQ (Quad_ptr quad){
    generate_RELATIONAL(quad, IF_GREATEREQ_IOPCODE);
}

void generate_IF_LESS (Quad_ptr quad){
    generate_RELATIONAL(quad, IF_LESS_IOPCODE);
}

void generate_IF_GREATER (Quad_ptr quad){
    generate_RELATIONAL(quad, IF_GREATER_IOPCODE);
}

void generate_CALL (Quad_ptr quad){
    generate_GENERIC(quad, CALL_IOPCODE);
}

void generate_PARAM (Quad_ptr quad){
    generate_GENERIC(quad, PUSHARG_IOPCODE);
}

void generate_RETURN (Quad_ptr quad){
    Instruction_ptr instruction = calloc(1, sizeof(struct Instruction));

    instruction->opcode = ASSIGN_IOPCODE;

    instruction->result = calloc(1, sizeof(struct Iarg));
    instruction->result->type = RETVAL_IARGTYPE;

    // The returned value lives in the quad's result field; a bare `return;` leaves
    // it NULL and returns nil.
    instruction->arg1 = calloc(1, sizeof(struct Iarg));
    if(quad->result){
        makeOperand(quad->result, instruction->arg1);
    } else {
        instruction->arg1->type = NIL_IARGTYPE;
    }

    instruction->line = quad->line;

    emitInstruction(instruction);
}

void generate_GETRETVAL (Quad_ptr quad){
    Instruction_ptr instruction = calloc(1, sizeof(struct Instruction));

    instruction->opcode = ASSIGN_IOPCODE;

    instruction->result = calloc(1, sizeof(struct Iarg));
    makeOperand(quad->result ,instruction->result);

    instruction->arg1 = calloc(1, sizeof(struct Iarg));
    instruction->arg1->type = RETVAL_IARGTYPE;

    instruction->line = quad->line;

    emitInstruction(instruction);
}

void generate_FUNCSTART (Quad_ptr quad){
    Instruction_ptr instruction = calloc(1, sizeof(struct Instruction));

    instruction->opcode = FUNCENTER_IOPCODE;

    instruction->result = calloc(1, sizeof(struct Iarg));
    makeOperand(quad->result ,instruction->result);

    instruction->line = quad->line;

    emitInstruction(instruction);
}

void generate_FUNCEND (Quad_ptr quad){
    Instruction_ptr instruction = calloc(1, sizeof(struct Instruction));

    instruction->opcode = FUNCEXIT_IOPCODE;

    instruction->result = calloc(1, sizeof(struct Iarg));
    makeOperand(quad->result, instruction->result);

    instruction->line = quad->line;

    emitInstruction(instruction);
}

void generate_NEWTABLE (Quad_ptr quad){
    generate_GENERIC(quad, NEWTABLE_IOPCODE);
}

void generate_TABLEGETELEM (Quad_ptr quad){
    generate_GENERIC(quad, TABLEGETELEM_IOPCODE);
}

void generate_TABLESETELEM (Quad_ptr quad){
    generate_GENERIC(quad, TABLESETELEM_IOPCODE);
}

void generate_JUMP (Quad_ptr quad){
    generate_RELATIONAL(quad, JUMP_IOPCODE);
}

void generate_NOP (Quad_ptr quad){
    Instruction_ptr instruction = calloc(1, sizeof(struct Instruction));
    instruction->opcode = NOP_IOPCODE;
    emitInstruction(instruction);
}

GeneratorFunc_ptr generators[] = {
    generate_ASSIGN,
    generate_ADD,
    generate_SUB,
    generate_MUL,
    generate_DIV,
    generate_MOD,
    generate_UMINUS,
    generate_IF_EQ,
    generate_IF_NOTEQ,
    generate_IF_LESSEQ,
    generate_IF_GREATEREQ,
    generate_IF_LESS,
    generate_IF_GREATER,
    generate_CALL,
    generate_PARAM,
    generate_RETURN,
    generate_GETRETVAL,
    generate_FUNCSTART,
    generate_FUNCEND,
    generate_NEWTABLE,
    generate_TABLEGETELEM,
    generate_TABLESETELEM,
    generate_JUMP,
    generate_NOP
};

void generateInstructions(void){
    unsigned int i;
    for(i = 1; i < nextQuadIndex; i++){
        (*generators[quadArr[i].op])(&quadArr[i]);
    }
}
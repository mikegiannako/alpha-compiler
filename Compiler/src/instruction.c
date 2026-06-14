#include "../include/instruction.h"
#include "../include/vm_code.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>

char* iopnames[] = {
    "assign",
    "add",
    "sub",
    "mul",
    "div",
    "mod",
    "if_eq",
    "if_noteq",
    "if_lesseq",
    "if_greatereq",
    "if_less",
    "if_greater",
    "call",
    "pusharg",
    "getretval",
    "funcenter",
    "funcexit",
    "newtable",
    "tablegetelem",
    "tablesetelem",
    "jump",
    "nop"
};

char* iargtypes[] = {
    "label",
    "global",
    "formal",
    "local",
    "number",
    "string",
    "bool",
    "nil",
    "userfunc",
    "libfunc",
    "retval"
};

// The instruction array is 1-indexed to stay 1:1 with the (also 1-indexed) quad
// array: quad i maps to instruction i. Index 0 is left unused so branch-target
// labels and function addresses carry over from quads without any shift.
#define INSTRUCTION_ARR_EMPTY_SIZE 1U

Instruction_ptr* instructions = NULL;
unsigned int currInstruction = INSTRUCTION_ARR_EMPTY_SIZE;
unsigned int instructionTableSize = 0;

void emitInstruction(Instruction_ptr instruction){
    if(currInstruction >= instructionTableSize){
        instructions = expandTable((void**)instructions, &instructionTableSize, currInstruction, sizeof(struct Instruction*));
    }

    instructions[currInstruction++] = instruction;
}

void instructions_FreeAll(void){
    // Instructions (and their Iargs) are calloc'd in the generator; the table
    // itself comes from expandTable's malloc/realloc. Skip the unused slot 0.
    for(unsigned int i = INSTRUCTION_ARR_EMPTY_SIZE; i < currInstruction; i++){
        free(instructions[i]->result);
        free(instructions[i]->arg1);
        free(instructions[i]->arg2);
        free(instructions[i]);
    }
    free(instructions);
    instructions = NULL;
    currInstruction = INSTRUCTION_ARR_EMPTY_SIZE;
    instructionTableSize = 0;
}


void printSingleInstruction(FILE* buffer, Instruction_ptr instruction, unsigned int instNo){
    fprintf(buffer, "%d: ", instNo);
    FillSpace(buffer, "", 6 - countDigits(instNo), " ");

    fprintf(buffer, "%-13s ", iopnames[instruction->opcode]);
    if(instruction->result){
        fprintf(buffer, "(%s),%-3d", iargtypes[instruction->result->type], instruction->result->val);
        FillSpace(buffer, iargtypes[instruction->result->type], 13 - 3, " ");
    } else {
        fprintf(buffer, "%-15s ", "");
    }

    if(instruction->arg1){
        fprintf(buffer, "(%s),%-3d", iargtypes[instruction->arg1->type], instruction->arg1->val);
        FillSpace(buffer, iargtypes[instruction->arg1->type], 13 - 3, " ");
    } else {
        fprintf(buffer, "%-15s ", "");
    }

    if(instruction->arg2){
        fprintf(buffer, "(%s),%-3d", iargtypes[instruction->arg2->type], instruction->arg2->val);
        FillSpace(buffer, iargtypes[instruction->arg2->type], 13 - 3, " ");
    } else {
        fprintf(buffer, "%-15s ", "");
    }

    fprintf(buffer, "%-3d\n", instruction->line);
}

void printToBuffer(FILE* buffer){
    for(unsigned int i = INSTRUCTION_ARR_EMPTY_SIZE; i < currInstruction; i++){
        printSingleInstruction(buffer, instructions[i], i);
    }

    // Print the constant tables
    fprintf(buffer, "\n\n Table of Constants\n");
    FillSpace(buffer, "", 21, "-");
    fprintf(buffer, "\nStrings:\n");
    for(unsigned int i = 0; i < totalStrConsts; i++){
        fprintf(buffer, "%d: %s\n", i, strConsts[i]);
    }

    fprintf(buffer, "\nNumbers:\n");
    for(unsigned int i = 0; i < totalNumConsts; i++){
        fprintf(buffer, "%d: %lf\n", i, numConsts[i]);
    }

    fprintf(buffer, "\nUser Functions:\n");
    for(unsigned int i = 0; i < totalUserFuncs; i++){
        fprintf(buffer, "%d: %s\n", i, userFuncs[i]->name);
    }

    fprintf(buffer, "\nLibrary Functions:\n");
    for(unsigned int i = 0; i < totalLibFuncs; i++){
        fprintf(buffer, "%d: %s\n", i, libFuncs[i]);
    }

    fclose(buffer);
}

void printInstructions(){
    FILE* buffer = fopen("instructions.txt", "w");

    #ifdef DEBUG
        printf("Total instructions: %d\n", currInstruction - INSTRUCTION_ARR_EMPTY_SIZE);
    #endif

    fprintf(stdout, "inst#   opcode        result          arg1            arg2            line\n");
    FillSpace(stdout, "", 14 + 14 + 14 + 14 + 7 + 11, "-");
    fprintf(stdout, "\n");

    printToBuffer(buffer);
    printToBuffer(stdout);
}
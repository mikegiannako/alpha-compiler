#include "../include/binary.h"
#include "../include/vm_code.h"
#include "../include/instruction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

static unsigned int MAGIC_NUMBER = 340200501;

void writeBinaryFile(char* filename){
    FILE* file = fopen(filename, "wb");
    if(!file){
        fprintf(stderr, "Could not open file %s\n", filename);
        exit(1);
    }

    fwrite(&MAGIC_NUMBER, sizeof(unsigned int), 1, file);
    
    fwrite(&totalNumConsts, sizeof(unsigned int), 1, file);
    for(unsigned int i = 0; i < totalNumConsts; i++){
        fwrite(&numConsts[i], sizeof(double), 1, file);
    }

    fwrite(&totalStrConsts, sizeof(unsigned int), 1, file);
    for(unsigned int i = 0; i < totalStrConsts; i++){
        unsigned int len = strlen(strConsts[i]);
        fwrite(&len, sizeof(unsigned int), 1, file);
        fwrite(strConsts[i], sizeof(char), len, file);
    }

    fwrite(&totalUserFuncs, sizeof(unsigned int), 1, file);
    for(unsigned int i = 0; i < totalUserFuncs; i++){
        fwrite(&userFuncs[i]->address, sizeof(unsigned int), 1, file);
        fwrite(&userFuncs[i]->localsSize, sizeof(unsigned int), 1, file);
        unsigned int len = strlen(userFuncs[i]->name);
        fwrite(&len, sizeof(unsigned int), 1, file);
        fwrite(userFuncs[i]->name, sizeof(char), len, file);
    }

    fwrite(&totalLibFuncs, sizeof(unsigned int), 1, file);
    for(unsigned int i = 0; i < totalLibFuncs; i++){
        unsigned int len = strlen(libFuncs[i]);
        fwrite(&len, sizeof(unsigned int), 1, file);
        fwrite(libFuncs[i], sizeof(char), len, file);
    }

    // The instruction array is 1-indexed (slot 0 is unused), so the real count is
    // currInstruction - 1 and the first instruction lives at index 1.
    unsigned int totalInstructions = currInstruction - 1;
    fwrite(&totalInstructions, sizeof(unsigned int), 1, file);
    for(unsigned int i = 1; i < currInstruction; i++){
        fwrite(&instructions[i]->opcode, sizeof(unsigned int), 1, file);
        
        if(instructions[i]->result){
            fwrite(&instructions[i]->result->type, sizeof(unsigned int), 1, file);
            fwrite(&instructions[i]->result->val, sizeof(unsigned int), 1, file);
        } else {
            Iargtype_enum type = NULL_IARGTYPE;
            fwrite(&type, sizeof(unsigned int), 1, file);
            unsigned int val = 0;
            fwrite(&val, sizeof(unsigned int), 1, file);
        }

        if(instructions[i]->arg1){
            fwrite(&instructions[i]->arg1->type, sizeof(unsigned int), 1, file);
            fwrite(&instructions[i]->arg1->val, sizeof(unsigned int), 1, file);
        } else {
            Iargtype_enum type = NULL_IARGTYPE;
            fwrite(&type, sizeof(unsigned int), 1, file);
            unsigned int val = 0;
            fwrite(&val, sizeof(unsigned int), 1, file);
        }

        if(instructions[i]->arg2){
            fwrite(&instructions[i]->arg2->type, sizeof(unsigned int), 1, file);
            fwrite(&instructions[i]->arg2->val, sizeof(unsigned int), 1, file);
        } else {
            Iargtype_enum type = NULL_IARGTYPE;
            fwrite(&type, sizeof(unsigned int), 1, file);
            unsigned int val = 0;
            fwrite(&val, sizeof(unsigned int), 1, file);
        }

        fwrite(&instructions[i]->line, sizeof(unsigned int), 1, file);
    }
}

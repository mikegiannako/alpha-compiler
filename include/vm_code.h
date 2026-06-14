#ifndef _VM_CODE_H_
#define _VM_CODE_H_

#include "intermediate.h"
#include "instruction.h"

typedef struct Userfunc {
    unsigned int address;
    unsigned int localsSize;
    char* name;
}* Userfunc_ptr;

// ----------------- Constants Tables -----------------
#define TABLE_EXPAND_SIZE 256
void* expandTable(void* table, unsigned int* tableSize, unsigned int total, unsigned int tableContentSize);
void* addNewConst(void* table, unsigned int* tableSize, unsigned int* total, void* newConst,
                         unsigned int tableContentSize, unsigned char (*cmp)(void*, void*, unsigned int),
                         void (*append)(void*, void* , unsigned int), unsigned int* index);

unsigned char compareNumConsts(void* table, void* newConst, unsigned int i);
unsigned char compareStrConsts(void* table, void* newConst, unsigned int i);
unsigned char compareUserFuncs(void* table, void* newConst, unsigned int i);

void appendNumConst(void* table, void* newConst, unsigned int i);
void appendStringConst(void* table, void* newConst, unsigned int i);
void appendUserFunc(void* table, void* newConst, unsigned int i);

extern double* numConsts;
extern unsigned int numConstsTableSize;
extern unsigned int totalNumConsts;

extern char** strConsts;
extern unsigned int strConstsTableSize;
extern unsigned int totalStrConsts;

extern Userfunc_ptr* userFuncs;
extern unsigned int userFuncsTableSize;
extern unsigned int totalUserFuncs;

extern char** libFuncs;
extern unsigned int libFuncsTableSize;
extern unsigned int totalLibFuncs;

// ----------------- Quads to Instructions -----------------

void makeOperand(Expr_ptr expr, Iarg_ptr arg);

void vmCode_FreeAll(void);

#endif
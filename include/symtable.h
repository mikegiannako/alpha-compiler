#ifndef _SYMTABLE_H_
#define _SYMTABLE_H_

#include "../include/generic_stack.h"

typedef enum scopeSpace scopeSpace_enum;
typedef enum symbolType symbolType_enum;
typedef struct symbolTableEntry* symbolTableEntry_ptr;
typedef struct symbolTable* symbolTable_ptr;

extern unsigned int scope;
extern int yylineno;

extern symbolTable_ptr globalSymbolTable;
extern symbolTable_ptr currentSymbolTable;


enum scopeSpace {
    PROGRAMVAR, FUNCTIONLOCAL, FORMALARG
};

enum symbolType {
    GLOBALVAR_SYMTYPE, LOCALVAR_SYMTYPE, FORMALVAR_SYMTYPE, USERFUNC_SYMTYPE, LIBFUNC_SYMTYPE
};

struct symbolTableEntry { 
    int isActive;
    const char *name;
    unsigned int scope;
    unsigned int line; 
    symbolType_enum type;

    scopeSpace_enum space;
    unsigned int offset;

    unsigned int iaddress;
    unsigned int totalLocals;
    unsigned int totalFormals;

    struct symbolTableEntry *next;
};

struct symbolTable {
    symbolTableEntry_ptr *buckets;
    unsigned int capacityIndex;
    unsigned int symbolCount;
    unsigned int scope;
    intPtrStack_ptr activeSymbolStack;

    struct symbolTable* next;
    struct symbolTable* prev;
};

symbolTable_ptr symbolTable_Init();
void symbolTable_FreeAll();
void symbolTable_EnterScope();
void symbolTable_ExitScope();
symbolTableEntry_ptr symbolTable_Insert(const char *name, symbolType_enum type);
symbolTableEntry_ptr symbolTable_Lookup(const char *name);
symbolTableEntry_ptr symbolTable_LocalLookup(const char *name);
symbolTableEntry_ptr symbolTable_GlobalLookup(const char *name);

void symbolTable_Print();

#endif
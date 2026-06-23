#ifndef _SYMTABLE_H_
#define _SYMTABLE_H_

#include <stdbool.h>
#include "../include/generic_stack.h"

typedef enum ScopeSpace ScopeSpace_enum;
typedef enum SymbolType SymbolType_enum;
typedef struct SymbolTableEntry* SymbolTableEntry_ptr;
typedef struct SymbolTable* SymbolTable_ptr;

extern unsigned int scope;
extern int yylineno;

extern SymbolTable_ptr globalSymbolTable;
extern SymbolTable_ptr currentSymbolTable;

enum ScopeSpace {
    PROGRAM_VAR_SPACE, FUNCTION_LOCAL_SPACE, FORMAL_ARG_SPACE
};

enum SymbolType {
    GLOBAL_VAR_SYMTYPE, LOCAL_VAR_SYMTYPE, FORMAL_VAR_SYMTYPE, USER_FUNC_SYMTYPE, LIB_FUNC_SYMTYPE
};

struct SymbolTableEntry { 
    int isActive;
    const char *name;
    unsigned int scope;
    unsigned int line; 
    SymbolType_enum type;

    ScopeSpace_enum space;
    unsigned int offset;

    unsigned int iaddress;
    unsigned int totalLocals;
    unsigned int totalFormals;

    // Set when the function body contains at least one `return` statement. Used by
    // the best-effort check that warns when a no-return function's result is used.
    bool hasReturn;

    struct SymbolTableEntry *next;
};

struct SymbolTable {
    SymbolTableEntry_ptr *buckets;
    unsigned int capacityIndex;
    unsigned int symbolCount;
    unsigned int scope;
    IntPtrStack_ptr activeSymbolStack;

    struct SymbolTable* next;
    struct SymbolTable* prev;
};

SymbolTable_ptr symbolTable_Init();
void symbolTable_FreeAll();
void symbolTable_EnterScope();
void symbolTable_ExitScope(bool clearScope);
SymbolTableEntry_ptr symbolTable_Insert(const char *name, SymbolType_enum type);
SymbolTableEntry_ptr symbolTable_Lookup(const char *name);
SymbolTableEntry_ptr symbolTable_LocalLookup(const char *name);
SymbolTableEntry_ptr symbolTable_GlobalLookup(const char *name);

void symbolTable_Print();

#endif
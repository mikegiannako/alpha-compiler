#include "../include/vm_code.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ----------------- Constants Tables -----------------
void* expandTable(void* table, unsigned int* tableSize, unsigned int total, unsigned int tableContentSize){
    assert(*tableSize <= total);

    void* result = NULL;
    if(total == 0){
        result = malloc(TABLE_EXPAND_SIZE * tableContentSize);
        *tableSize = TABLE_EXPAND_SIZE;
    } else{
        result = realloc(table, (total + TABLE_EXPAND_SIZE) * tableContentSize);
        *tableSize += TABLE_EXPAND_SIZE;
    }

    return result;
}

void* addNewConst(void* table, unsigned int* tableSize, unsigned int* total, void* newConst,
                         unsigned int tableContentSize, unsigned char (*cmp)(void*, void*, unsigned int),
                         void (*append)(void*, void* , unsigned int), unsigned int* index)
{
    unsigned int i;
    for(i = 0; i < *total; i++){
        if(cmp(table, newConst, i)){
            *index = i;
            return table;
        }
    }

    if(*tableSize == *total){
        table = expandTable(table, tableSize, *total, tableContentSize);
    }

    append(table, newConst, *total);

    *index = (*total)++;

    return table;
}

unsigned char compareNumConsts(void* table, void* newConst, unsigned int i){
    return *((double*)table + i) == *((double*)newConst);
}

unsigned char compareStrConsts(void* table, void* newConst, unsigned int i){
    return !strcmp(*((char**)table + i), ((char*)newConst));
}

unsigned char compareUserFuncs(void* table, void* newConst, unsigned int i){
    return ((Userfunc_ptr*)table)[i]->address == ((Userfunc_ptr)newConst)->address;
}

void appendNumConst(void* table, void* newConst, unsigned int i){
    *((double*)table + i) = *((double*)newConst);
}

void appendStringConst(void* table, void* newConst, unsigned int i){
    ((char**)table)[i] = ((char*)newConst);
}

void appendUserFunc(void* table, void* newConst, unsigned int i){
    ((Userfunc_ptr*)table)[i] = calloc(1, sizeof(struct Userfunc));
    ((Userfunc_ptr*)table)[i]->address = ((Userfunc_ptr)newConst)->address;
    ((Userfunc_ptr*)table)[i]->localsSize = ((Userfunc_ptr)newConst)->localsSize;
    ((Userfunc_ptr*)table)[i]->name = malloc(strlen(((Userfunc_ptr)newConst)->name) + 1);
    strcpy(((Userfunc_ptr*)table)[i]->name, ((Userfunc_ptr)newConst)->name);
}

double* numConsts = NULL;
unsigned int numConstsTableSize = 0;
unsigned int totalNumConsts = 0;

char** strConsts = NULL;
unsigned int strConstsTableSize = 0;
unsigned int totalStrConsts = 0;

Userfunc_ptr* userFuncs = NULL;
unsigned int userFuncsTableSize = 0;
unsigned int totalUserFuncs = 0;

char** libFuncs = NULL;
unsigned int libFuncsTableSize = 0;
unsigned int totalLibFuncs = 0;

void vmCode_FreeAll(void){
    // numConsts: plain double array.
    free(numConsts);
    numConsts = NULL;
    numConstsTableSize = totalNumConsts = 0;

    // strConsts: each element is a strdup'd copy made in makeOperand.
    for(unsigned int i = 0; i < totalStrConsts; i++){
        free(strConsts[i]);
    }
    free(strConsts);
    strConsts = NULL;
    strConstsTableSize = totalStrConsts = 0;

    // userFuncs: each struct and its name are copies (see appendUserFunc).
    for(unsigned int i = 0; i < totalUserFuncs; i++){
        free(userFuncs[i]->name);
        free(userFuncs[i]);
    }
    free(userFuncs);
    userFuncs = NULL;
    userFuncsTableSize = totalUserFuncs = 0;

    // libFuncs: elements are borrowed sym->name pointers (owned by the symbol
    // table), so free only the array, not the strings it holds.
    free(libFuncs);
    libFuncs = NULL;
    libFuncsTableSize = totalLibFuncs = 0;
}

// ----------------- Quads to Instructions -----------------

void makeOperand(Expr_ptr expr, Iarg_ptr arg){
    switch(expr->type){
        case VAR_EXPRTYPE:
        case TABLE_ITEM_EXPRTYPE:
        case ARITH_EXPR_EXPRTYPE:
        case BOOL_EXPR_EXPRTYPE:
        case NEW_TABLE_EXPRTYPE:
        case ASSIGN_EXPR_EXPRTYPE: {
            arg->val = expr->sym->offset;

            switch(expr->sym->space){
                case PROGRAM_VAR_SPACE: arg->type = GLOBAL_IARGTYPE; break;
                case FUNCTION_LOCAL_SPACE: arg->type = LOCAL_IARGTYPE; break;
                case FORMAL_ARG_SPACE: arg->type = FORMAL_IARGTYPE; break;
                default: assert(0);
            }

            break;
        }
        case CONST_BOOL_EXPRTYPE: {
            arg->val = expr->boolConst;
            arg->type = BOOL_IARGTYPE;
            break;
        }
        case CONST_STRING_EXPRTYPE: {
            // appendStringConst stores the pointer directly, so the table takes
            // ownership only when the string is new. On a dedup hit nothing stores
            // the copy, so free it to avoid leaking.
            char* strCopy = strdup(expr->strConst);
            unsigned int prevTotal = totalStrConsts;
            strConsts = addNewConst((void*)strConsts, &strConstsTableSize, &totalStrConsts, (void*)strCopy, \
                                    sizeof(char*), compareStrConsts, appendStringConst, &arg->val);
            if(totalStrConsts == prevTotal) free(strCopy);
            arg->type = STRING_IARGTYPE;
            break;
        }
        case CONST_NUM_EXPRTYPE: {
            numConsts = addNewConst((void*)numConsts, &numConstsTableSize, &totalNumConsts, (void*)&expr->numConst, \
                                    sizeof(double), compareNumConsts, appendNumConst, &arg->val);
            arg->type = NUMBER_IARGTYPE;
            break;
        }
        case NIL_EXPRTYPE: {
            arg->type = NIL_IARGTYPE;
            break;
        }
        case PROGRAM_FUNC_EXPRTYPE: {
            Userfunc_ptr userfunc = calloc(1, sizeof(struct Userfunc));

            userfunc->address = expr->sym->iaddress;
            userfunc->localsSize = expr->sym->totalLocals;
            userfunc->name = malloc(strlen(expr->sym->name) + 1);
            strcpy(userfunc->name, expr->sym->name);

            userFuncs = addNewConst((void*)userFuncs, &userFuncsTableSize, &totalUserFuncs, (void*)userfunc, \
                                     sizeof(Userfunc_ptr), compareUserFuncs, appendUserFunc, &arg->val);

            // appendUserFunc deep-copies the struct and its name, so the temp is
            // no longer needed (whether it was appended or matched an existing one).
            free(userfunc->name);
            free(userfunc);

            arg->type = USERFUNC_IARGTYPE;
            break;
        }
        case LIBRARY_FUNC_EXPRTYPE: {
            libFuncs = addNewConst((void*)libFuncs, &libFuncsTableSize, &totalLibFuncs, (void*)expr->sym->name, \
                                    sizeof(char*), compareStrConsts, appendStringConst, &arg->val);
            arg->type = LIBFUNC_IARGTYPE;
            break;
        }
        default: {
            printf("expr->type: %d\n", expr->type);
            assert(0);
        }
    }
}
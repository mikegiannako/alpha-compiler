#include "../include/symtable.h"
#include "../include/memory_operations.h"
#include "../include/error_macros.h"
#include "../include/generic_stack.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

// Defined in intermediate.c
ScopeSpace_enum currScopeSpace();
unsigned int currScopeOffset();
void incCurScopeOffset();

#define SCOPE_ACTIVE 1
#define SCOPE_INACTIVE 0

SymbolTable_ptr globalSymbolTable;
SymbolTable_ptr currentSymbolTable;

#define HASH_MULTIPLIER 65599

static unsigned int symbolTable_Hash(const char *key, unsigned int size){
    unsigned int ui;
    unsigned int uiHash = 0U;
    for (ui = 0U; key[ui] != '\0'; ui++)
        uiHash = uiHash * HASH_MULTIPLIER + key[ui];
    return uiHash % size;
}

#define MIN_SIZE 256
#define MAX_SIZE 32768
int tableSizes[] = {MIN_SIZE, 512, 1024, 2048, 4096, 8192, 16384, MAX_SIZE};

SymbolTable_ptr symbolTable_Init(){
    SymbolTable_ptr table = safeCalloc(1, sizeof(struct SymbolTable), "creating Symbol Table");
    table->capacityIndex = 0;
    table->buckets = safeCalloc(tableSizes[table->capacityIndex], sizeof(SymbolTableEntry_ptr), "creating Symbol Table Buckets");
    return table;
}

void symbolTableEntry_Destroy(SymbolTableEntry_ptr* entry){
    safeFree(&((*entry)->name), "freeing Symbol Table Entry name");
    safeFree(entry, "freeing Symbol Table Entry");
}

void symbolTable_Destroy(SymbolTable_ptr* table){
    for(unsigned int i = 0; i < tableSizes[(*table)->capacityIndex]; i++){
        SymbolTableEntry_ptr current = (*table)->buckets[i];
        while(current){
            SymbolTableEntry_ptr toDelete = current;
            current = current->next;
            symbolTableEntry_Destroy(&toDelete);
        }
    }
    intPtrStack_Clear(&((*table)->activeSymbolStack));
    safeFree(&((*table)->buckets), "freeing Symbol Table Buckets");
    safeFree(table, "freeing Symbol Table");
}

void symbolTable_FreeAll(){
    SymbolTable_ptr temp = globalSymbolTable;

    while(temp != NULL){
        SymbolTable_ptr next = temp->next;
        symbolTable_Destroy(&temp);
        temp = next;
    }
}

void symbolTable_EnterScope(){
    if(currentSymbolTable->next == NULL){
        currentSymbolTable->next = symbolTable_Init();
        currentSymbolTable->next->prev = currentSymbolTable;
    }

    currentSymbolTable = currentSymbolTable->next;
}

void symbolTable_ExitScope(bool clearScope){
    if(currentSymbolTable->prev == NULL){
        ERROR_MSG("SYMTABLE", "Trying to exit global scope");
    }

    while(clearScope && !intPtrStack_IsEmpty(currentSymbolTable->activeSymbolStack)){
        *intPtrStack_Pop(&currentSymbolTable->activeSymbolStack) = SCOPE_INACTIVE;
    }

    currentSymbolTable = currentSymbolTable->prev;
}


void symbolTable_Resize(){
    if(tableSizes[currentSymbolTable->capacityIndex + 1] >= MAX_SIZE){
        return;
    }

    unsigned int oldCapacity = tableSizes[currentSymbolTable->capacityIndex];
    unsigned int newCapacity = tableSizes[++currentSymbolTable->capacityIndex];

    SymbolTableEntry_ptr* newBuckets = safeCalloc(newCapacity, sizeof(SymbolTableEntry_ptr), "creating resized Symbol Table Buckets");

    for(unsigned int i = 0; i < oldCapacity; i++){
        SymbolTableEntry_ptr entry = currentSymbolTable->buckets[i];
        while(entry){
            SymbolTableEntry_ptr nextEntry = entry->next;
            unsigned int newIndex = symbolTable_Hash(entry->name, newCapacity);
            entry->next = newBuckets[newIndex];
            newBuckets[newIndex] = entry;
            entry = nextEntry;
        }
    }

    safeFree(&(currentSymbolTable->buckets), "freeing old Symbol Table Buckets");
    currentSymbolTable->buckets = newBuckets;
}

SymbolTableEntry_ptr symbolTable_Insert(const char *name, SymbolType_enum type){

    if(currentSymbolTable->symbolCount == currentSymbolTable->capacityIndex){
        symbolTable_Resize();
    }

    unsigned int index = symbolTable_Hash(name, tableSizes[currentSymbolTable->capacityIndex]);
    SymbolTableEntry_ptr newEntry = safeCalloc(1, sizeof(struct SymbolTableEntry), "creating Symbol Table Entry");
    newEntry->isActive = SCOPE_ACTIVE;

    // Keeping the active symbols in a stack for easy/fast hiding
    intPtrStack_Push(&currentSymbolTable->activeSymbolStack, &newEntry->isActive);

    newEntry->name = safeStrDup(name, "duplicating Symbol Table Entry name");
    newEntry->scope = scope;
    newEntry->line = yylineno;
    newEntry->type = type;

    newEntry->next = currentSymbolTable->buckets[index];
    currentSymbolTable->buckets[index] = newEntry;
    currentSymbolTable->symbolCount++;

    if(type == LIB_FUNC_SYMTYPE) newEntry->totalFormals = 0;
    else if(type == USER_FUNC_SYMTYPE) {}
    else {
        newEntry->space = currScopeSpace();
        newEntry->offset = currScopeOffset();
        incCurScopeOffset();
    }

    return newEntry;
}

SymbolTableEntry_ptr symbolTable_Lookup(const char *name){
    SymbolTable_ptr tempTable = currentSymbolTable;
    while(tempTable){
        unsigned int index = symbolTable_Hash(name, tableSizes[tempTable->capacityIndex]);
        SymbolTableEntry_ptr entry = tempTable->buckets[index];
        while(entry != NULL){
            if(entry->isActive == SCOPE_INACTIVE){
                entry = entry->next;
                continue;
            }

            if(strcmp(entry->name, name) == 0){
                return entry;
            }

            entry = entry->next;
        }

        tempTable = tempTable->prev;
    }

    return NULL;
}

SymbolTableEntry_ptr symbolTable_LocalLookup(const char *name){
    unsigned int index = symbolTable_Hash(name, tableSizes[currentSymbolTable->capacityIndex]);
    SymbolTableEntry_ptr entry = currentSymbolTable->buckets[index];
    while(entry != NULL){
        if(entry->isActive == SCOPE_INACTIVE){
            entry = entry->next;
            continue;
        }

        if(strcmp(entry->name, name) == 0){
            return entry;
        }

        entry = entry->next;
    }

    return NULL;
}

SymbolTableEntry_ptr symbolTable_GlobalLookup(const char *name){
    unsigned int index = symbolTable_Hash(name, tableSizes[globalSymbolTable->capacityIndex]);
    SymbolTableEntry_ptr entry = globalSymbolTable->buckets[index];
    while(entry != NULL){
        if(entry->isActive == SCOPE_INACTIVE){
            entry = entry->next;
            continue;
        }

        if(strcmp(entry->name, name) == 0 && entry->scope == 0){
            return entry;
        }
        
        entry = entry->next;
    }
    return NULL;
}

static const char* symbolType_ToString(SymbolType_enum type){
    switch(type){
        case GLOBAL_VAR_SYMTYPE: return "GLOBALVAR";
        case LOCAL_VAR_SYMTYPE: return "LOCALVAR";
        case FORMAL_VAR_SYMTYPE: return "FORMALVAR";
        case USER_FUNC_SYMTYPE: return "USERFUNC";
        case LIB_FUNC_SYMTYPE: return "LIBFUNC";
        default: return "UNKNOWN";
    }
}

// static const char* scopeSpace_ToString(ScopeSpace_enum space){
//     switch(space){
//         case PROGRAM_VAR_SPACE: return "PROGRAM_VAR_SPACE";
//         case FUNCTION_LOCAL_SPACE: return "FUNCTION_LOCAL_SPACE";
//         case FORMAL_ARG_SPACE: return "FORMAL_ARG_SPACE";
//         default: return "UNKNOWN";
//     }
// }

void symbolTable_Print(){
    printf("\n");
    printf("===========================================\n");
    printf("                SYMBOL TABLE\n");
    printf("===========================================\n\n");

    SymbolTable_ptr currentScope = globalSymbolTable;
    unsigned int scopeNum = 0;

    while(currentScope != NULL){
        /* Skip if this is the last scope and it's empty */
        if(currentScope->next == NULL && currentScope->symbolCount == 0){
            break;
        }

        if(scopeNum == 0){
            printf("\033[33m        ----- Scope %u ", scopeNum);
            printf("(Global) ");
        } else {
            printf("\033[33m            ----- Scope %u ", scopeNum);
        }
        printf("-----\033[0m\n\n");

        if(currentScope->symbolCount == 0){
            printf("  (empty)\n\n");
            currentScope = currentScope->next;
            scopeNum++;
            continue;
        }

        printf("%-20s %-15s %s\n",
               "Name", "Type", "Line");
        printf("-------------------------------------------\n");

        for(unsigned int i = 0; i < tableSizes[currentScope->capacityIndex]; i++){
            SymbolTableEntry_ptr entry = currentScope->buckets[i];
            while(entry != NULL){
                if(entry->type == LIB_FUNC_SYMTYPE){
                    /* For library functions, print only name and type in purple */
                    printf("%-20s \033[35m%-15s\033[0m\n",
                           entry->name,
                           symbolType_ToString(entry->type));
                } else {
                    /* For all other types, print full information */
                    printf("%-20s %-15s %-6u ",
                           entry->name,
                           symbolType_ToString(entry->type),
                           entry->line);

                    // if(entry->type == USER_FUNC_SYMTYPE){
                    //     printf("%-16s %-8s ", "-", "-");
                    //     printf("addr:%-4u locals:%-2u formals:%-2u",
                    //            entry->iaddress,
                    //            entry->totalLocals,
                    //            entry->totalFormals);
                    // } else {
                    //     printf("%-16s %-8u ",
                    //            scopeSpace_ToString(entry->space),
                    //            entry->offset);
                    //     printf("-");
                    // }

                    printf("\n");
                }

                entry = entry->next;
            }
        }

        printf("\n");
        currentScope = currentScope->next;
        scopeNum++;
    }

    printf("===========================================\n\n");
}
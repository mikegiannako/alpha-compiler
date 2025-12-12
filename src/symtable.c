#include "../include/symtable.h"
#include "../include/memory_operations.h"
#include "../include/error_macros.h"
#include "../include/generic_stack.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SCOPE_ACTIVE 1
#define SCOPE_INACTIVE 0

symbolTable_ptr globalSymbolTable;
symbolTable_ptr currentSymbolTable;

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

symbolTable_ptr symbolTable_Init(){
    symbolTable_ptr table = safeCalloc(1, sizeof(struct symbolTable), "creating Symbol Table");
    table->capacityIndex = 0;
    table->buckets = safeCalloc(tableSizes[table->capacityIndex], sizeof(symbolTableEntry_ptr), "creating Symbol Table Buckets");
    return table;
}

void symbolTableEntry_Destroy(symbolTableEntry_ptr* entry){
    safeFree(&((*entry)->name), "freeing Symbol Table Entry name");
    safeFree(entry, "freeing Symbol Table Entry");
}

void symbolTable_Destroy(symbolTable_ptr* table){
    for(unsigned int i = 0; i < tableSizes[(*table)->capacityIndex]; i++){
        symbolTableEntry_ptr current = (*table)->buckets[i];
        while(current){
            symbolTableEntry_ptr toDelete = current;
            current = current->next;
            symbolTableEntry_Destroy(&toDelete);
        }
    }
    intPtrStack_Clear(&((*table)->activeSymbolStack));
    safeFree(&((*table)->buckets), "freeing Symbol Table Buckets");
    safeFree(table, "freeing Symbol Table");
}

void symbolTable_FreeAll(){
    symbolTable_ptr temp = globalSymbolTable;

    while(temp != NULL){
        symbolTable_ptr next = temp->next;
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

void symbolTable_ExitScope(){
    if(currentSymbolTable->prev == NULL){
        ERROR_MSG("SYMTABLE", "Trying to exit global scope");
    }

    while(!intPtrStack_IsEmpty(currentSymbolTable->activeSymbolStack)){
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

    symbolTableEntry_ptr* newBuckets = safeCalloc(newCapacity, sizeof(symbolTableEntry_ptr), "creating resized Symbol Table Buckets");

    for(unsigned int i = 0; i < oldCapacity; i++){
        symbolTableEntry_ptr entry = currentSymbolTable->buckets[i];
        while(entry){
            symbolTableEntry_ptr nextEntry = entry->next;
            unsigned int newIndex = symbolTable_Hash(entry->name, newCapacity);
            entry->next = newBuckets[newIndex];
            newBuckets[newIndex] = entry;
            entry = nextEntry;
        }
    }

    safeFree(&(currentSymbolTable->buckets), "freeing old Symbol Table Buckets");
    currentSymbolTable->buckets = newBuckets;
}

symbolTableEntry_ptr symbolTable_Insert(const char *name, symbolType_enum type){

    if(currentSymbolTable->symbolCount == currentSymbolTable->capacityIndex){
        symbolTable_Resize();
    }

    unsigned int index = symbolTable_Hash(name, tableSizes[currentSymbolTable->capacityIndex]);
    symbolTableEntry_ptr newEntry = safeCalloc(1, sizeof(struct symbolTableEntry), "creating Symbol Table Entry");
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

    if(type == LIBFUNC_SYMTYPE) newEntry->totalFormals = 0;
    else if(type == USERFUNC_SYMTYPE) {}
    else {
        // newEntry->space = currScopeSpace();
        // newEntry->offset = currScopeOffset();
        // incCurScopeOffset();
    }

    return newEntry;
}

symbolTableEntry_ptr symbolTable_Lookup(const char *name){
    symbolTable_ptr tempTable = currentSymbolTable;
    while(tempTable){
        unsigned int index = symbolTable_Hash(name, tableSizes[tempTable->capacityIndex]);
        symbolTableEntry_ptr entry = tempTable->buckets[index];
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

symbolTableEntry_ptr symbolTable_LocalLookup(const char *name){
    unsigned int index = symbolTable_Hash(name, tableSizes[currentSymbolTable->capacityIndex]);
    symbolTableEntry_ptr entry = currentSymbolTable->buckets[index];
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

symbolTableEntry_ptr symbolTable_GlobalLookup(const char *name){
    unsigned int index = symbolTable_Hash(name, tableSizes[globalSymbolTable->capacityIndex]);
    symbolTableEntry_ptr entry = globalSymbolTable->buckets[index];
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

void symbolTable_Print();
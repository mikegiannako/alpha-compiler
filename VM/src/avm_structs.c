#include "../include/avm_structs.h"
#include "../include/binary_reader.h"
#include "../include/avm_dispatcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

char* VMMemCellTypeStrings[] = {
    "number",
    "string",
    "bool",
    "table",
    "userfunc",
    "libfunc",
    "nil",
    "undefined"
};

VMMemCell_ptr ExecutionStack[AVM_STACK_SIZE];

// The cells are allocated as one contiguous block so that the pointers stored in
// ExecutionStack[] increase monotonically with the stack index. Several execute*
// handlers assert an lvalue/table is a live stack slot via pointer-range checks
// (ExecutionStack[top] >= cell > ExecutionStack[SP]); that ordering is only valid
// when the cells are contiguous, not individually calloc'd.
static struct VMMemCell* stackCells = NULL;

void InitializeStack(void){
    stackCells = safeCalloc(AVM_STACK_SIZE, sizeof(struct VMMemCell), "allocating the VM execution stack block");
    for (unsigned i = 0; i < AVM_STACK_SIZE; i++){
        ExecutionStack[i] = &stackCells[i];
        ExecutionStack[i]->type = UNDEFINED_MEMCELL;
    }

    VM_BP = AVM_STACK_SIZE - globalCount - 1;
    VM_SP = VM_BP;
}

void VMMemCell_Clear(VMMemCell_ptr cell){
    if (cell == NULL) {
        return; // If cell is NULL, there's nothing to clear
    }

    switch (cell->type) {
        case NUMBER_MEMCELL:
            // No action needed for number
            break;
        case STRING_MEMCELL:
            if (cell->data.string != NULL) {
                safeFree(&cell->data.string, "freeing a string memcell's string");
            }
            break;
        case BOOL_MEMCELL:
            // No action needed for boolean
            break;
        case TABLE_MEMCELL:
            VMTable_DecRefCount(&cell->data.table);
            cell->data.table = NULL; // Set to NULL after decrementing refCount
            break;
        case USERFUNC_MEMCELL:
            // No action needed for user function
            break;
        case LIBFUNC_MEMCELL:
            if(cell->data.libfunc != NULL) {
                safeFree(&cell->data.libfunc, "freeing a libfunc memcell's name");
            }
            break;
        case NIL_MEMCELL:
        case UNDEFINED_MEMCELL:
            // No action needed for nil or undefined
            break;
    }

    VM_WIPEOUT(*cell); // Wipe out the memory cell
    cell->type = UNDEFINED_MEMCELL; // Set type to undefined
}


// ------------------------------------------------------------------------------------------------------

VMTable_ptr VMTable_New(){
    VMTable_ptr table = safeCalloc(1, sizeof(struct VMTable), "allocating a VM table");

    table->refCount = 0;
    table->total = 0;

    for (int i = 0; i < 4; i++) {
        VMTable_BucketInit(&table->buckets[i], VMTABLE_HASHSIZE);
    }
    VMTable_BucketInit(&table->buckets[BOOL_INDEXED_TABLE], 2);

    return table;
} 

// void VMTable_Free(VMTable_ptr table);

void VMTable_BucketInit(VMTableBucket_ptr** buckets, unsigned size){
    *buckets = safeCalloc(size, sizeof(struct VMTableBucket*), "allocating a VM table bucket array");

    for (unsigned i = 0; i < size; i++) {
        (*buckets)[i] = NULL; // Initialize each bucket to NULL
    }
}

void VMTable_BucketFree(VMTableBucket_ptr* buckets, unsigned size){
    if(buckets == NULL)
        return; // If buckets is NULL, there's nothing to free

    for (unsigned i = 0; i < size; i++) {
        VMTableBucket_ptr current = buckets[i];
        while (current) {
            VMTableBucket_ptr next = current->next;
            VMMemCell_Clear(current->key);
            VMMemCell_Clear(current->value);
            safeFree(&current->key, "freeing a table bucket key cell");
            safeFree(&current->value, "freeing a table bucket value cell");
            safeFree(&current, "freeing a table bucket");
            current = next;
        }
    }

    safeFree(&buckets, "freeing a table bucket array");
}

unsigned char VMMemCell_Equals(VMMemCell_ptr a, VMMemCell_ptr b) {
    if (a == NULL || b == NULL) {
        return 0; // If either cell is NULL, they are not equal
    }

    if (a->type != b->type) {
        return 0; // Different types cannot be equal
    }

    switch (a->type) {
        case NUMBER_MEMCELL:
            return a->data.number == b->data.number;
        case STRING_MEMCELL:
            return strcmp(a->data.string, b->data.string) == 0;
        case BOOL_MEMCELL:
            return a->data.boolean == b->data.boolean;
        case USERFUNC_MEMCELL:
            return a->data.userfunc == b->data.userfunc;
        case LIBFUNC_MEMCELL:
            return strcmp(a->data.libfunc, b->data.libfunc) == 0;
        default:
            return 0; // Unsupported type
    }
}

unsigned VMTable_HashString(const char* str) {
    unsigned hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash;
}

VMTableBucket_ptr* FindBucketArray(VMTable_ptr table, VMMemCell_ptr key) {
    if (table == NULL || key == NULL) {
        ERROR_MSG("VM", "VMTable_Find: Table or key is NULL");
    }

    unsigned bucket_index;
    VMTableBucket_ptr* bucket_array;

    switch (key->type) {
        case NUMBER_MEMCELL:
            bucket_index = (unsigned)key->data.number % VMTABLE_HASHSIZE;
            bucket_array = table->buckets[NUM_INDEXED_TABLE];
            break;
        case STRING_MEMCELL:
            bucket_index = VMTable_HashString(key->data.string) % VMTABLE_HASHSIZE;
            bucket_array = table->buckets[STR_INDEXED_TABLE];
            break;
        case BOOL_MEMCELL:
            bucket_index = key->data.boolean ? 1 : 0;
            bucket_array = table->buckets[BOOL_INDEXED_TABLE];
            break;
        case USERFUNC_MEMCELL:
            bucket_index = vm_program.user_functions[key->data.userfunc].address % VMTABLE_HASHSIZE;
            bucket_array = table->buckets[USERFUNC_INDEXED_TABLE];
            break;
        case LIBFUNC_MEMCELL:
            bucket_index = VMTable_HashString(key->data.libfunc) % VMTABLE_HASHSIZE;
            bucket_array = table->buckets[LIBFUNC_INDEXED_TABLE];
            break;
        default:
            ERROR_MSG("VM", "VMTable_Find: Unsupported key type '%s'", VMMemCellTypeStrings[key->type]);
    }

    return &bucket_array[bucket_index];
}

void VMTable_Remove(VMTable_ptr table, VMMemCell_ptr key) {
    if (table == NULL || key == NULL) {
        ERROR_MSG("VM", "VMTable_Remove: Table or key is NULL");
    }


    VMTableBucket_ptr* ptr_to_bucket = FindBucketArray(table, key);
    VMTableBucket_ptr current = *ptr_to_bucket;
    assert(current != NULL);

    VMTableBucket_ptr prev = NULL;
    while (current) {
        if (VMMemCell_Equals(current->key, key)) {
            if (prev) {
                prev->next = current->next;
            } else {
                *ptr_to_bucket = current->next;
            }
            VMMemCell_Clear(current->key);
            VMMemCell_Clear(current->value);
            safeFree(&current->key, "freeing a removed table bucket key cell");
            safeFree(&current->value, "freeing a removed table bucket value cell");
            safeFree(&current, "freeing a removed table bucket");
            table->total--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

VMMemCell_ptr VMTable_Get(VMTable_ptr table, VMMemCell_ptr key) {
    if (table == NULL || key == NULL) {
        ERROR_MSG("VM", "VMTable_Get: Table or key is NULL");
    }

    VMTableBucket_ptr current = *FindBucketArray(table, key);
    // assert(current != NULL);

    while (current) {
        if (VMMemCell_Equals(current->key, key)) {
            return current->value;
        }
        current = current->next;
    }

    return NULL; // Key not found
}

void VMTable_Set(VMTable_ptr table, VMMemCell_ptr key, VMMemCell_ptr value) {
    if (table == NULL || key == NULL || value == NULL) {
        ERROR_MSG("VM", "VMTable_Set: Table, key, or value is NULL");
        return;
    }

    // Search for existing key
    VMTableBucket_ptr* ptr_to_bucket = FindBucketArray(table, key);
    VMTableBucket_ptr current = *ptr_to_bucket;
    
    while (current) {
        if (VMMemCell_Equals(current->key, key)) {
            if(value->type == NIL_MEMCELL) {
                // If existing value is nil, remove it
                VMTable_Remove(table, key);
                skipNextTableGetElem = 1; // If value is nil, skip next table get element
                return;
            }

            // Key exists, update value
            VMMemCell_Clear(current->value);
            VMMemCell_Assign(current->value, value);
            return;
        }
        current = current->next;
    }

    if(value->type == NIL_MEMCELL) {
        skipNextTableGetElem = 1; // If value is nil, skip next table get element
        return;
    }

    // Key doesn't exist, create new bucket
    VMTableBucket_ptr new_bucket = safeCalloc(1, sizeof(struct VMTableBucket), "allocating a new table bucket");

    new_bucket->key = safeCalloc(1, sizeof(struct VMMemCell), "allocating a table bucket key cell");
    new_bucket->value = safeCalloc(1, sizeof(struct VMMemCell), "allocating a table bucket value cell");

    VMMemCell_Assign(new_bucket->key, key);
    VMMemCell_Assign(new_bucket->value, value);
    
    // Insert at the beginning of the bucket chain
    new_bucket->next = *ptr_to_bucket;
    *ptr_to_bucket = new_bucket;

    table->total++;
}

void VMTable_IncRefCount(VMTable_ptr table){
    assert(table != NULL); // Ensure table is not NULL
    assert(table->refCount < UINT_MAX); // Ensure refCount does not overflow

    table->refCount++;
}

void VMTable_DecRefCount(VMTable_ptr* table){
    assert(table != NULL); // Ensure table is not NULL
    assert(*table != NULL); // Ensure *table is not NULL
    if((*table)->refCount <= 0) return;

    (*table)->refCount--;
    if ((*table)->refCount == 0) {
        VMTable_Destroy(*table);
        *table = NULL;
    }
}

void VMTable_Destroy(VMTable_ptr table){
    if (table == NULL) {
        return; // If table is NULL, there's nothing to destroy
    }

    for (int i = 0; i < 4; i++) {
        VMTable_BucketFree(table->buckets[i], VMTABLE_HASHSIZE);
    }
    VMTable_BucketFree(table->buckets[BOOL_INDEXED_TABLE], 2);

    safeFree(&table, "freeing a VM table");
}


void VMProgram_Cleanup(void) {
    // Free instructions
    if (vm_program.instructions) {
        safeFree(&vm_program.instructions, "freeing VM instructions table");
    }

    // Free string constants
    if (vm_program.string_constants) {
        for (unsigned i = 0; i < vm_program.string_constants_count; i++) {
            safeFree(&vm_program.string_constants[i], "freeing a VM string constant");
        }
        safeFree(&vm_program.string_constants, "freeing VM string constants table");
    }

    // Free number constants
    if (vm_program.number_constants) {
        safeFree(&vm_program.number_constants, "freeing VM number constants table");
    }

    // Free library functions
    if (vm_program.library_functions) {
        for (unsigned i = 0; i < vm_program.library_functions_count; i++) {
            safeFree(&vm_program.library_functions[i], "freeing a VM library function name");
        }
        safeFree(&vm_program.library_functions, "freeing VM library functions table");
    }

    // Free user functions
    if (vm_program.user_functions) {
        for (unsigned i = 0; i < vm_program.user_functions_count; i++) {
            safeFree(&vm_program.user_functions[i].name, "freeing a VM user function name");
        }
        safeFree(&vm_program.user_functions, "freeing VM user functions table");
    }

    // Reset structure
    memset(&vm_program, 0, sizeof(VMProgram));


    // Clear interior data (strings/tables) of every stack cell, then free the
    // single contiguous block backing them. The cells themselves are not freed
    // individually since they are slices of one allocation.
    if (stackCells) {
        for (unsigned i = 0; i < AVM_STACK_SIZE; i++) {
            VMMemCell_Clear(ExecutionStack[i]);
            ExecutionStack[i] = NULL;
        }
        safeFree(&stackCells, "freeing the VM execution stack block");
    }
}
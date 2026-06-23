#ifndef _AVM_STRUCTS_H_
#define _AVM_STRUCTS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "binary_reader.h"

// Compile-time debug logging. With -DDEBUG these expand to a printf; in a normal
// build they vanish, so the VM emits only actual program output. Mirrors the
// compiler's RULE_PRINT pattern. `make debug` enables -DDEBUG.
#ifdef DEBUG
    #define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...) do { } while (0)
#endif

typedef enum VMMemCellType {
    NUMBER_MEMCELL    = 0,
    STRING_MEMCELL    = 1,
    BOOL_MEMCELL      = 2,
    TABLE_MEMCELL     = 3,
    USERFUNC_MEMCELL  = 4,
    LIBFUNC_MEMCELL   = 5,
    NIL_MEMCELL       = 6,
    UNDEFINED_MEMCELL = 7 
} VMMemCellType_enum;

extern char* VMMemCellTypeStrings[];

typedef struct VMTable* VMTable_ptr;

typedef struct VMMemCell {
    VMMemCellType_enum type;        // Type of the memory cell
    union {
        double number;              // For NUMBER_MEMCELL
        char* string;               // For STRING_MEMCELL
        unsigned char boolean;      // For BOOL_MEMCELL
        VMTable_ptr table;          // For TABLE_MEMCELL
        unsigned userfunc;          // For USERFUNC_MEMCELL
        char* libfunc;              // For LIBFUNC_MEMCELL
    } data;                         // Union to hold different types of data
}* VMMemCell_ptr;

#define AVM_STACK_SIZE 4096
#define VM_WIPEOUT(m) memset(&(m), 0, sizeof(m))

extern VMMemCell_ptr ExecutionStack[AVM_STACK_SIZE];
void InitializeStack(void);
void VMMemCell_Clear(VMMemCell_ptr cell);

// ------------------------------------------------------------------------------------------------------

typedef enum VMTableIndexingType {
    STR_INDEXED_TABLE = 0,      // Indexed by integers
    NUM_INDEXED_TABLE = 1,      // Indexed by strings
    USERFUNC_INDEXED_TABLE = 2, // Indexed by user functions
    LIBFUNC_INDEXED_TABLE = 3,  // Indexed by library functions
    BOOL_INDEXED_TABLE = 4,     // Indexed by booleans
} VMTableIndexingType_enum;

typedef struct VMTableBucket {
    VMMemCell_ptr key;                  // Key of the bucket
    VMMemCell_ptr value;                // Value of the bucket
    struct VMTableBucket* next;  // Pointer to the next bucket in the chain
}* VMTableBucket_ptr;

#define VMTABLE_HASHSIZE 211

typedef struct VMTable {
    unsigned refCount;            // Reference count for garbage collection
    VMTableBucket_ptr* buckets[5];    // Array of buckets for different indexing types
    unsigned total;               // Total number of elements in the table
}* VMTable_ptr;

extern unsigned char skipNextTableGetElem;

VMTable_ptr VMTable_New();
void VMTable_Destroy(VMTable_ptr table);
void VMTable_BucketInit(VMTableBucket_ptr** buckets, unsigned size);
void VMTable_BucketFree(VMTableBucket_ptr* buckets, unsigned size);
unsigned char VMMemCell_Equals(VMMemCell_ptr a, VMMemCell_ptr b);
VMMemCell_ptr VMTable_Get(VMTable_ptr table, VMMemCell_ptr key);
void VMTable_Set(VMTable_ptr table, VMMemCell_ptr key, VMMemCell_ptr value);
void VMTable_IncRefCount(VMTable_ptr table);
void VMTable_DecRefCount(VMTable_ptr* table);

void VMProgram_Cleanup(void);


#endif 
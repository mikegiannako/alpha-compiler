#ifndef _BINARY_READER_H_
#define _BINARY_READER_H_

#include <stdio.h>

// Shared memory + error helpers (repo-root common/, on the -I path).
#include "memory_operations.h"
#include "error_macros.h"

typedef enum VMArgType_enum {
    LABEL_IARGTYPE = 0,      // = label_a
    GLOBAL_IARGTYPE,         // = global_a  
    FORMAL_IARGTYPE,         // = formal_a
    LOCAL_IARGTYPE,          // = local_a
    NUMBER_IARGTYPE,         // = number_a
    STRING_IARGTYPE,         // = string_a
    BOOL_IARGTYPE,           // = bool_a
    NIL_IARGTYPE,            // = nil_a
    USERFUNC_IARGTYPE,       // = userfunc_a
    LIBFUNC_IARGTYPE,        // = libfunc_a
    RETVAL_IARGTYPE,         // = retval_a
    NULL_IARGTYPE            // = null_a
} VMArgType_enum;

typedef struct VMArg {
    VMArgType_enum type;
    unsigned val;
} VMArg;

typedef enum VMOpcode_enum {
    ASSIGN_IOPCODE = 0,     // = assign_i
    ADD_IOPCODE,            // = add_i
    SUB_IOPCODE,            // = sub_i
    MUL_IOPCODE,            // = mul_i
    DIV_IOPCODE,            // = div_i
    MOD_IOPCODE,            // = mod_i
    IF_EQ_IOPCODE,          // = if_eq_i
    IF_NOTEQ_IOPCODE,       // = if_noteq_i
    IF_LESSEQ_IOPCODE,      // = if_lesseq_i
    IF_GREATEREQ_IOPCODE,   // = if_greatereq_i
    IF_LESS_IOPCODE,        // = if_less_i
    IF_GREATER_IOPCODE,     // = if_greater_i
    CALL_IOPCODE,           // = call_i
    PUSHARG_IOPCODE,        // = pusharg_i
    FUNCENTER_IOPCODE,      // = funcenter_i
    FUNCEXIT_IOPCODE,       // = funcexit_i
    NEWTABLE_IOPCODE,       // = newtable_i
    TABLEGETELEM_IOPCODE,   // = tablegetelem_i
    TABLESETELEM_IOPCODE,   // = tablesetelem_i
    JUMP_IOPCODE,           // = jump_i
    NOP_IOPCODE             // = nop_i
} VMOpcode_enum;

extern char* VMOpcodeStrings[];

typedef struct VMInstruction {
    VMOpcode_enum opcode;
    VMArg result;
    VMArg arg1;
    VMArg arg2;
    unsigned srcLine;
} VMInstruction;

typedef struct VMUserFunction {
    char* name;
    unsigned localsSize;
    unsigned address;
} VMUserFunction;

typedef struct VMProgram {
    // Instructions
    VMInstruction* instructions;
    unsigned instruction_count;
    
    // Constant tables
    char** string_constants;
    unsigned string_constants_count;
    
    double* number_constants;
    unsigned number_constants_count;
    
    char** library_functions;
    unsigned library_functions_count;
    
    VMUserFunction* user_functions;
    unsigned user_functions_count;
} VMProgram;

// Global VM program instance
extern VMProgram vm_program;
extern unsigned globalCount;

/**
 * Load a binary file and populate the VM program structure
 * @param filename Binary file to load
 * @return 0 on success, -1 on error
 */
int binaryReader_Load(const char* filename);

/**
 * Print information about the loaded VM program
 */
void binaryReader_PrintInfo(void);

/**
 * Access functions for VM execution
 */
static inline VMInstruction* VMProgram_GetInstruction(unsigned address) {
    if (address == 0 || address > vm_program.instruction_count) {
        return NULL;
    }
    return &vm_program.instructions[address];
}

static inline const char* VMProgram_GetStringConst(unsigned index) {
    if (index >= vm_program.string_constants_count) {
        return NULL;
    }
    return vm_program.string_constants[index];
}

static inline double VMProgram_GetNumberConst(unsigned index) {
    if (index >= vm_program.number_constants_count) {
        return 0.0;
    }
    return vm_program.number_constants[index];
}

static inline const char* VMProgram_GetLibFunc(unsigned index) {
    if (index >= vm_program.library_functions_count) {
        return NULL;
    }
    return vm_program.library_functions[index];
}

static inline VMUserFunction* VMProgram_GetUserFunc(unsigned index) {
    if (index >= vm_program.user_functions_count) {
        return NULL;
    }
    return &vm_program.user_functions[index];
}

#endif // BINARY_READER_H
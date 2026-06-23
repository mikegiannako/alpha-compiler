#include "../include/binary_reader.h"
#include "../include/avm_structs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC_NUMBER 340200501

unsigned globalCount = 0; // Global instruction count

// Global VM data structures
VMProgram vm_program = {0};

// Spellings kept in lockstep with the Compiler's iopnames[] (instruction.c) so
// debug output matches across the two tools.
char* VMOpcodeStrings[] = {
    "assign",
    "add",
    "sub",
    "mul",
    "div",
    "mod",
    "if_eq",
    "if_noteq",
    "if_lesseq",
    "if_greatereq",
    "if_less",
    "if_greater",
    "call",
    "pusharg",
    "funcenter",
    "funcexit",
    "newtable",
    "tablegetelem",
    "tablesetelem",
    "jump",
    "nop"
};

// Read a single unsigned. Returns 1 on success, 0 on failure.
static int binaryReader_ReadUint(FILE* file, unsigned* out) {
    return fread(out, sizeof(unsigned), 1, file) == 1;
}

// Read a string from file (length + raw chars, no terminator on disk).
// The writer stores `len` followed by exactly `len` bytes; we allocate len+1
// and add the null terminator so the result is a usable C string.
static char* binaryReader_ReadString(FILE* file) {
    unsigned len;
    if (fread(&len, sizeof(unsigned), 1, file) != 1) {
        return NULL;
    }

    char* str = safeCalloc(len + 1, sizeof(char), "reading a string from the binary file");

    if (len > 0 && fread(str, sizeof(char), len, file) != len) {
        safeFree(&str, "freeing a partially-read binary string");
        return NULL;
    }

    str[len] = '\0';
    return str;
}

// Helpers to keep binaryReader_Load readable. Each returns nonzero on failure so
// the caller can bail out and clean up.
#define READ_FAIL(file, msg, ...) do { \
        fprintf(stderr, "Error: " msg "\n", ##__VA_ARGS__); \
        fclose(file); \
        VMProgram_Cleanup(); \
        return -1; \
    } while (0)

int binaryReader_Load(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open binary file '%s'\n", filename);
        return -1;
    }

    DEBUG_PRINT("Loading binary file: %s\n", filename);

    // --- Magic number ---
    unsigned magic;
    if (!binaryReader_ReadUint(file, &magic)) {
        fprintf(stderr, "Error: Cannot read magic number\n");
        fclose(file);
        return -1;
    }
    if (magic != MAGIC_NUMBER) {
        fprintf(stderr, "Error: Invalid magic number %u (expected %u)\n", magic, MAGIC_NUMBER);
        fclose(file);
        return -1;
    }
    DEBUG_PRINT("Valid binary file detected\n");

    // The writer emits the sections in this exact order (see Compiler binary.c):
    //   magic, number consts, string consts, user funcs, lib funcs, instructions.
    // Each table is preceded by its own count, so counts are NOT contiguous.

    // --- Number constants ---
    if (!binaryReader_ReadUint(file, &vm_program.number_constants_count)) {
        READ_FAIL(file, "Cannot read number constants count");
    }
    if (vm_program.number_constants_count > 0) {
        vm_program.number_constants = safeCalloc(vm_program.number_constants_count, sizeof(double), "allocating VM number constants table");
        if (fread(vm_program.number_constants, sizeof(double),
                  vm_program.number_constants_count, file) != vm_program.number_constants_count) {
            READ_FAIL(file, "Cannot read number constants");
        }
    }
    DEBUG_PRINT("Loaded %u number constants\n", vm_program.number_constants_count);

    // --- String constants ---
    if (!binaryReader_ReadUint(file, &vm_program.string_constants_count)) {
        READ_FAIL(file, "Cannot read string constants count");
    }
    if (vm_program.string_constants_count > 0) {
        vm_program.string_constants = safeCalloc(vm_program.string_constants_count, sizeof(char*), "allocating VM string constants table");
        for (unsigned i = 0; i < vm_program.string_constants_count; i++) {
            vm_program.string_constants[i] = binaryReader_ReadString(file);
            if (!vm_program.string_constants[i]) READ_FAIL(file, "Cannot read string constant %u", i);
        }
    }
    DEBUG_PRINT("Loaded %u string constants\n", vm_program.string_constants_count);

    // --- User functions ---  writer order per entry: address, localsSize, name
    if (!binaryReader_ReadUint(file, &vm_program.user_functions_count)) {
        READ_FAIL(file, "Cannot read user functions count");
    }
    if (vm_program.user_functions_count > 0) {
        vm_program.user_functions = safeCalloc(vm_program.user_functions_count, sizeof(VMUserFunction), "allocating VM user functions table");
        for (unsigned i = 0; i < vm_program.user_functions_count; i++) {
            if (!binaryReader_ReadUint(file, &vm_program.user_functions[i].address)) {
                READ_FAIL(file, "Cannot read user function %u address", i);
            }
            if (!binaryReader_ReadUint(file, &vm_program.user_functions[i].localsSize)) {
                READ_FAIL(file, "Cannot read user function %u localsSize", i);
            }
            vm_program.user_functions[i].name = binaryReader_ReadString(file);
            if (!vm_program.user_functions[i].name) READ_FAIL(file, "Cannot read user function %u name", i);
        }
    }
    DEBUG_PRINT("Loaded %u user functions\n", vm_program.user_functions_count);

    // --- Library functions ---
    if (!binaryReader_ReadUint(file, &vm_program.library_functions_count)) {
        READ_FAIL(file, "Cannot read library functions count");
    }
    if (vm_program.library_functions_count > 0) {
        vm_program.library_functions = safeCalloc(vm_program.library_functions_count, sizeof(char*), "allocating VM library functions table");
        for (unsigned i = 0; i < vm_program.library_functions_count; i++) {
            vm_program.library_functions[i] = binaryReader_ReadString(file);
            if (!vm_program.library_functions[i]) READ_FAIL(file, "Cannot read library function %u", i);
        }
    }
    DEBUG_PRINT("Loaded %u library functions\n", vm_program.library_functions_count);

    // --- Instructions ---  field-by-field per instruction (opcode, 3 args, line).
    // Stored 1-indexed (slot 0 unused) to match the writer's 1-indexed stream.
    if (!binaryReader_ReadUint(file, &vm_program.instruction_count)) {
        READ_FAIL(file, "Cannot read instruction count");
    }
    if (vm_program.instruction_count > 0) {
        vm_program.instructions = safeCalloc(vm_program.instruction_count + 1, sizeof(VMInstruction), "allocating VM instructions table");

        for (unsigned i = 1; i <= vm_program.instruction_count; i++) {
            VMInstruction* instr = &vm_program.instructions[i];
            unsigned opcode, rtype, rval, a1type, a1val, a2type, a2val, line;
            if (!binaryReader_ReadUint(file, &opcode) ||
                !binaryReader_ReadUint(file, &rtype)  || !binaryReader_ReadUint(file, &rval)  ||
                !binaryReader_ReadUint(file, &a1type) || !binaryReader_ReadUint(file, &a1val) ||
                !binaryReader_ReadUint(file, &a2type) || !binaryReader_ReadUint(file, &a2val) ||
                !binaryReader_ReadUint(file, &line)) {
                READ_FAIL(file, "Cannot read instruction %u", i);
            }
            instr->opcode      = (VMOpcode_enum)opcode;
            instr->result.type = (VMArgType_enum)rtype;  instr->result.val = rval;
            instr->arg1.type   = (VMArgType_enum)a1type; instr->arg1.val   = a1val;
            instr->arg2.type   = (VMArgType_enum)a2type; instr->arg2.val   = a2val;
            instr->srcLine     = line;
        }

        // Largest global offset referenced sets the global-variable count.
        globalCount = 0;
        for (unsigned i = 1; i <= vm_program.instruction_count; i++) {
            VMInstruction* instr = &vm_program.instructions[i];
            if (instr->result.type == GLOBAL_IARGTYPE && instr->result.val >= globalCount) globalCount = instr->result.val + 1;
            if (instr->arg1.type   == GLOBAL_IARGTYPE && instr->arg1.val   >= globalCount) globalCount = instr->arg1.val + 1;
            if (instr->arg2.type   == GLOBAL_IARGTYPE && instr->arg2.val   >= globalCount) globalCount = instr->arg2.val + 1;
        }
        DEBUG_PRINT("Global variable count set to: %u\n", globalCount);
    }
    DEBUG_PRINT("Loaded %u instructions\n", vm_program.instruction_count);

    fclose(file);
    DEBUG_PRINT("Binary file loaded successfully!\n");
    return 0;
}

void binaryReader_PrintInfo(void) {
    printf("\n=== VM PROGRAM INFORMATION ===\n");
    printf("Instructions: %u\n", vm_program.instruction_count);
    printf("String Constants: %u\n", vm_program.string_constants_count);
    printf("Number Constants: %u\n", vm_program.number_constants_count);
    printf("Library Functions: %u\n", vm_program.library_functions_count);
    printf("User Functions: %u\n", vm_program.user_functions_count);
    
    // Print first few instructions as example
    printf("\nInstructions:\n");
    for (unsigned i = 1; i <= vm_program.instruction_count; i++) {
        VMInstruction* instr = &vm_program.instructions[i];
        printf("  %u: opcode=%s, result=(%u,%u), arg1=(%u,%u), arg2=(%u,%u), line=%u\n",
               i, VMOpcodeStrings[instr->opcode],
               instr->result.type, instr->result.val,
               instr->arg1.type, instr->arg1.val,
               instr->arg2.type, instr->arg2.val,
               instr->srcLine);
    }
    
    // Print first few constants as examples
    if (vm_program.string_constants_count > 0) {
        printf("\nFirst few string constants:\n");
        for (unsigned i = 0; i < vm_program.string_constants_count; i++) {
            printf("  %u: \"%s\"\n", i, vm_program.string_constants[i]);
        }
    }
    
    if (vm_program.number_constants_count > 0) {
        printf("\nFirst few number constants:\n");
        for (unsigned i = 0; i < vm_program.number_constants_count; i++) {
            printf("  %u: %g\n", i, vm_program.number_constants[i]);
        }
    }
    
    if (vm_program.user_functions_count > 0) {
        printf("\nUser functions:\n");
        for (unsigned i = 0; i < vm_program.user_functions_count; i++) {
            printf("  %u: %s (localsSize=%u, address=%u)\n", i,
                   vm_program.user_functions[i].name,
                   vm_program.user_functions[i].localsSize,
                   vm_program.user_functions[i].address);
        }
    }

    if (vm_program.library_functions_count > 0) {
        printf("\nLibrary functions:\n");
        for (unsigned i = 0; i < vm_program.library_functions_count; i++) {
            printf("  %u: %s\n", i, vm_program.library_functions[i]);
        }
    }
    
    printf("\n");
}
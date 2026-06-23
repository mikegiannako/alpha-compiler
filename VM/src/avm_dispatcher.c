#include "../include/avm_structs.h"
#include "../include/binary_reader.h"
#include "../include/avm_dispatcher.h"
#include "../include/avm_trace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

VMMemCell_ptr ax, bx, cx, retval;
unsigned  VM_SP, VM_PC, VM_BP;
unsigned char skipNextTableGetElem;

VMMemCell_ptr VMInstruction_TranslateOperand(VMArg arg, VMMemCell_ptr reg){
    switch(arg.type){
        case GLOBAL_IARGTYPE: return ExecutionStack[AVM_STACK_SIZE - 1 - arg.val];
        case LOCAL_IARGTYPE:  return ExecutionStack[VM_BP - arg.val];
        case FORMAL_IARGTYPE: return ExecutionStack[VM_BP + VM_STACK_ENV_SIZE + arg.val + 1];
        case RETVAL_IARGTYPE: return retval;
        case NUMBER_IARGTYPE: {
            reg->type = NUMBER_MEMCELL;
            reg->data.number = vm_program.number_constants[arg.val];
            return reg;
        }
        case STRING_IARGTYPE: {
            reg->type = STRING_MEMCELL;
            reg->data.string = vm_program.string_constants[arg.val];
            if (!reg->data.string) {
                fprintf(stderr, "Error: Cannot allocate memory for string constant\n");
                exit(EXIT_FAILURE);
            }
            return reg;
        }
        case BOOL_IARGTYPE: {
            reg->type = BOOL_MEMCELL;
            reg->data.boolean = (unsigned char)arg.val; // Assuming arg.val is 0 or 1
            return reg;
        }
        case NIL_IARGTYPE: {
            reg->type = NIL_MEMCELL;
            return reg;
        }
        case USERFUNC_IARGTYPE: {
            reg->type = USERFUNC_MEMCELL;
            assert(arg.val < vm_program.user_functions_count);
            reg->data.userfunc = arg.val;
            assert(vm_program.user_functions[arg.val].address < vm_program.instruction_count);
            return reg;
        }
        case LIBFUNC_IARGTYPE: {
            reg->type = LIBFUNC_MEMCELL;
            unsigned index = arg.val;
            assert(index < vm_program.library_functions_count);
            reg->data.libfunc = vm_program.library_functions[index];
            if (!reg->data.libfunc) {
                fprintf(stderr, "Error: Cannot allocate memory for library function\n");
                exit(EXIT_FAILURE);
            }
            return reg;
        }
        default: {
            fprintf(stderr, "Error: Invalid VMArgType_enum %d\n", arg.type);
            exit(EXIT_FAILURE);
        }
    }
}

static VMExecuteFunc_ptr VMInstruction_executeFuncs[] = {
    VMInstruction_ExecuteAssign,
    VMInstruction_ExecuteArithmetic,
    VMInstruction_ExecuteArithmetic,
    VMInstruction_ExecuteArithmetic,
    VMInstruction_ExecuteArithmetic,
    VMInstruction_ExecuteArithmetic,
    VMInstruction_ExecuteIfEq,
    VMInstruction_ExecuteIfNotEq,
    VMInstruction_ExecuteComparison,
    VMInstruction_ExecuteComparison,
    VMInstruction_ExecuteComparison,
    VMInstruction_ExecuteComparison,
    VMInstruction_ExecuteCall,
    VMInstruction_ExecutePushArg,
    VMInstruction_ExecuteFuncEnter,
    VMInstruction_ExecuteFuncExit,
    VMInstruction_ExecuteNewTable,
    VMInstruction_ExecuteTableGetElem,
    VMInstruction_ExecuteTableSetElem,
    VMInstruction_ExecuteJump,
    VMInstruction_ExecuteNop
};

unsigned char executionFinished = 0;
unsigned currLine = 0;
VMInstruction currInstr;


void VMInstruction_ExecuteCycle(void){
    if(executionFinished) {
        return;
    }
    
    // Instructions are 1-indexed (valid range 1..instruction_count), so the
    // program ends once PC moves past the last instruction.
    if(VM_PC > vm_program.instruction_count) {
        executionFinished = 1;
        return;
    }

    assert(VM_PC >= 1 && VM_PC <= vm_program.instruction_count);
    currInstr = vm_program.instructions[VM_PC];

    assert(currInstr.opcode >= 0 && currInstr.opcode <= (unsigned) NOP_IOPCODE);

    if(currInstr.srcLine) currLine = currInstr.srcLine;

    VMTrace_Step(&currInstr, VM_PC);

    if(skipNextTableGetElem) {
        // A nil tablesetelem deletes the key, so the redundant "assignment result"
        // tablegetelem the compiler emits right after would read a now-missing key;
        // skip it. Mind that the two opcodes lay out their operands differently:
        //   tablesetelem: result=table, arg1=key,   arg2=value
        //   tablegetelem: result=dest,  arg1=table,  arg2=key
        skipNextTableGetElem = 0;
        if(currInstr.opcode == TABLEGETELEM_IOPCODE){
            VMInstruction prevInstr = vm_program.instructions[VM_PC - 1];
            if(prevInstr.opcode == TABLESETELEM_IOPCODE &&
                prevInstr.result.val  == currInstr.arg1.val  &&   // same table
                prevInstr.result.type == currInstr.arg1.type &&
                prevInstr.arg1.val    == currInstr.arg2.val  &&   // same key
                prevInstr.arg1.type   == currInstr.arg2.type) {

                VM_PC++;
                return;
            }
        }
    }
    
    unsigned oldPC = VM_PC;

    (*VMInstruction_executeFuncs[currInstr.opcode])(&currInstr);

    if(VM_PC == oldPC) VM_PC++;
}

// --------------------------------------------------------------------------------------------------


void VMInstruction_ExecuteAssign(VMInstruction* instr) {
    VMMemCell_ptr lv = VMInstruction_TranslateOperand(instr->result, NULL);
    VMMemCell_ptr rv = VMInstruction_TranslateOperand(instr->arg1, ax);
    
    assert(lv && ((ExecutionStack[AVM_STACK_SIZE - 1] >= lv && lv > ExecutionStack[VM_SP]) || lv == retval));
    assert(rv);

    VMMemCell_Assign(lv, rv);

    // This assign is a "getretval" when it reads from retval -- the move the
    // compiler emits right after a call to capture its return value. Once that
    // value has been handed to its destination, release retval (back to nil) so a
    // later call that never sets its own return value can't leak this stale one.
    // (Also drops retval's reference to a returned table now that the caller owns it.)
    if(instr->arg1.type == RETVAL_IARGTYPE){
        VMMemCell_Clear(retval);
        retval->type = NIL_MEMCELL;
    }
}

// ----------------------------------------------------------


typedef double (*VMArithmeticFunc_ptr)(double, double);

double add_arithmetic(double a, double b) {
    return a + b;
}
double sub_arithmetic(double a, double b) {
    return a - b;
}

double mul_arithmetic(double a, double b) {
    return a * b;
}

double div_arithmetic(double a, double b) {
    if (b == 0) {
        ERROR_MSG("VM", "Division by zero at line %d", currLine);
    }
    return a / b;
}

double mod_arithmetic(double a, double b) {
    if (b == 0) {
        ERROR_MSG("VM", "Modulo by zero at line %d", currLine);
    }
    return fmod(a, b);
}

VMArithmeticFunc_ptr arithmeticFuncs[] = {
    add_arithmetic,
    sub_arithmetic,
    mul_arithmetic,
    div_arithmetic,
    mod_arithmetic
};

void VMInstruction_ExecuteArithmetic(VMInstruction* instr) {
    VMMemCell_ptr lv = VMInstruction_TranslateOperand(instr->result, NULL);
    VMMemCell_ptr rv1 = VMInstruction_TranslateOperand(instr->arg1, ax);
    VMMemCell_ptr rv2 = VMInstruction_TranslateOperand(instr->arg2, bx);
    
    assert(lv && ((ExecutionStack[AVM_STACK_SIZE - 1] >= lv && lv > ExecutionStack[VM_SP]) || lv == retval));
    assert(rv1 && rv2);

    // '+' doubles as string concatenation when both operands are strings
    // (e.g. "foo" + "bar" -> "foobar").
    if(instr->opcode == ADD_IOPCODE && rv1->type == STRING_MEMCELL && rv2->type == STRING_MEMCELL) {
        size_t len1 = strlen(rv1->data.string);
        size_t len2 = strlen(rv2->data.string);
        char* concat = safeCalloc(len1 + len2 + 1, sizeof(char), "allocating a string for VM concatenation");
        memcpy(concat, rv1->data.string, len1);
        memcpy(concat + len1, rv2->data.string, len2);

        // lv may alias rv1/rv2 (e.g. s = s + x); the source bytes are already
        // copied into 'concat', so clearing lv now is safe.
        VMMemCell_Clear(lv);
        lv->type = STRING_MEMCELL;
        lv->data.string = concat;
        return;
    }

    if(rv1->type != NUMBER_MEMCELL || rv2->type != NUMBER_MEMCELL) {
        ERROR_MSG("VM", "Arithmetic operation on non-number types: '%s' and '%s' at line %d", VMMemCellTypeStrings[rv1->type], VMMemCellTypeStrings[rv2->type], currLine);
    }

    double result = arithmeticFuncs[instr->opcode - ADD_IOPCODE](rv1->data.number, rv2->data.number);

    VMMemCell_Clear(lv);

    lv->type = NUMBER_MEMCELL;
    lv->data.number = result;
};

// ----------------------------------------------------------

typedef unsigned char (*VMToBoolFunc_ptr)(VMMemCell_ptr);

unsigned char VMMemCell_ToBoolNumber(VMMemCell_ptr cell) {
    return (cell->data.number != 0.0) ? 1 : 0;
}

unsigned char VMMemCell_ToBoolString(VMMemCell_ptr cell) {
    return (strlen(cell->data.string) > 0) ? 1 : 0;
}

unsigned char VMMemCell_ToBoolBool(VMMemCell_ptr cell) {
    return cell->data.boolean;
}

unsigned char VMMemCell_TrueToBool(VMMemCell_ptr cell){
    (void)cell;
    return 1;
}

unsigned char VMMemCell_FalseToBool(VMMemCell_ptr cell){
    (void)cell;
    return 0;
}

VMToBoolFunc_ptr toBoolFuncs[] = {
    VMMemCell_ToBoolNumber, // For NUMBER_MEMCELL
    VMMemCell_ToBoolString, // For STRING_MEMCELL
    VMMemCell_ToBoolBool,   // For BOOL_MEMCELL
    VMMemCell_TrueToBool,   // For TABLE_MEMCELL (assumed always true)
    VMMemCell_TrueToBool,   // For USERFUNC_MEMCELL (assumed always true)
    VMMemCell_TrueToBool,   // For LIBFUNC_MEMCELL (assumed always true)
    VMMemCell_FalseToBool,  // For NIL_MEMCELL (always false)
    VMMemCell_FalseToBool   // For UNDEFINED_MEMCELL (always false)
};

unsigned char VMMemCell_ToBool(VMMemCell_ptr cell){
    assert(cell->type >= NUMBER_MEMCELL && cell->type <= UNDEFINED_MEMCELL);
    return (*toBoolFuncs[cell->type])(cell);
}

void VMInstruction_ExecuteIfEq(VMInstruction* instr) {
    assert(instr->result.type == LABEL_IARGTYPE);

    VMMemCell_ptr op1 = VMInstruction_TranslateOperand(instr->arg1, ax);
    VMMemCell_ptr op2 = VMInstruction_TranslateOperand(instr->arg2, bx);
    
    assert(op1 && op2);

    unsigned char result = 0;

    if(op1->type == UNDEFINED_MEMCELL || op2->type == UNDEFINED_MEMCELL) {
        ERROR_MSG("VM", "Comparing undefined values at line %d", currLine);
    } else if(op1->type == BOOL_MEMCELL || op2->type == BOOL_MEMCELL) {
        result = (VMMemCell_ToBool(op1) == VMMemCell_ToBool(op2));
    } else if (op1->type == NIL_MEMCELL || op2->type == NIL_MEMCELL) {
        result = (op1->type == NIL_MEMCELL && op2->type == NIL_MEMCELL);
    } else if(op1->type != op2->type) {
        ERROR_MSG("VM", "Cannot compare different types: '%s' and '%s' at line %d", VMMemCellTypeStrings[op1->type], VMMemCellTypeStrings[op2->type], currLine);
    } else {
        switch(op1->type) {
            case NUMBER_MEMCELL:
                result = (op1->data.number == op2->data.number);
                break;
            case STRING_MEMCELL:
                result = (strcmp(op1->data.string, op2->data.string) == 0);
                break;
            case TABLE_MEMCELL:
                result = (op1->data.table == op2->data.table);
                break;
            case USERFUNC_MEMCELL:
                result = (op1->data.userfunc == op2->data.userfunc);
                break;
            case LIBFUNC_MEMCELL:
                result = (strcmp(op1->data.libfunc, op2->data.libfunc) == 0);
                break;
            default:
                assert(0 && "Invalid memory cell type for comparison");
                break;
        }
    }

    if(!executionFinished && result) VM_PC = instr->result.val;
}

void VMInstruction_ExecuteIfNotEq(VMInstruction* instr) {
    assert(instr->result.type == LABEL_IARGTYPE);

    VMMemCell_ptr op1 = VMInstruction_TranslateOperand(instr->arg1, ax);
    VMMemCell_ptr op2 = VMInstruction_TranslateOperand(instr->arg2, bx);
    
    assert(op1 && op2);

    unsigned char result = 0;

    if(op1->type == UNDEFINED_MEMCELL || op2->type == UNDEFINED_MEMCELL) {
        ERROR_MSG("VM", "Comparing undefined values at line %d", currLine);
    } else if(op1->type == BOOL_MEMCELL || op2->type == BOOL_MEMCELL) {
        result = (VMMemCell_ToBool(op1) != VMMemCell_ToBool(op2));
    } else if (op1->type == NIL_MEMCELL || op2->type == NIL_MEMCELL) {
        result = (op1->type != NIL_MEMCELL || op2->type != NIL_MEMCELL);
    } else if(op1->type != op2->type) {
        ERROR_MSG("VM", "Cannot compare different types: '%s' and '%s' at line %d", VMMemCellTypeStrings[op1->type], VMMemCellTypeStrings[op2->type], currLine);
    } else {
        switch(op1->type) {
            case NUMBER_MEMCELL:
                result = (op1->data.number != op2->data.number);
                break;
            case STRING_MEMCELL:
                result = (strcmp(op1->data.string, op2->data.string) != 0);
                break;
            case TABLE_MEMCELL:
                result = (op1->data.table != op2->data.table);
                break;
            case USERFUNC_MEMCELL:
                result = (op1->data.userfunc != op2->data.userfunc);
                break;
            case LIBFUNC_MEMCELL:
                result = (strcmp(op1->data.libfunc, op2->data.libfunc) != 0);
                break;
            default:
                assert(0 && "Invalid memory cell type for comparison");
                break;
        }
    }

    if(!executionFinished && result) VM_PC = instr->result.val;
}

// ----------------------------------------------------------

typedef unsigned char (*VMComparisonFunc_ptr)(double, double);

unsigned char lesseq_comparison(double a, double b) {
    return a <= b;
}

unsigned char greatereq_comparison(double a, double b) {
    return a >= b;
}

unsigned char less_comparison(double a, double b) {
    return a < b;
}

unsigned char greater_comparison(double a, double b) {
    return a > b;
}

VMComparisonFunc_ptr comparisonFuncs[] = {
    lesseq_comparison,
    greatereq_comparison,
    less_comparison,
    greater_comparison
};

void VMInstruction_ExecuteComparison(VMInstruction* instr) {
    assert(instr->result.type == LABEL_IARGTYPE);

    VMMemCell_ptr op1 = VMInstruction_TranslateOperand(instr->arg1, ax);
    VMMemCell_ptr op2 = VMInstruction_TranslateOperand(instr->arg2, bx);
    
    assert(op1 && op2);

    if(op1->type != NUMBER_MEMCELL || op2->type != NUMBER_MEMCELL) {
        ERROR_MSG("VM", "Comparison operation on non-number types: '%s' and '%s' at line %d", VMMemCellTypeStrings[op1->type], VMMemCellTypeStrings[op2->type], currLine);
    }

    unsigned char result = comparisonFuncs[instr->opcode - IF_LESSEQ_IOPCODE](op1->data.number, op2->data.number);

    if(!executionFinished && result) VM_PC = instr->result.val;
}


// ----------------------------------------------------------



void VMInstruction_ExecuteCall(VMInstruction* instr) {
    VMMemCell_ptr func = VMInstruction_TranslateOperand(instr->arg1, ax);
    
    assert(func);

    switch(func->type) {
        case USERFUNC_MEMCELL: {
            VMCall_SaveEnvironment();
            VM_PC = vm_program.user_functions[func->data.userfunc].address;

            assert(VM_PC < vm_program.instruction_count);
            assert(vm_program.instructions[VM_PC].opcode == FUNCENTER_IOPCODE);

            break;
        }
        case LIBFUNC_MEMCELL: {
            VMCall_LibFunc(func->data.libfunc);
            break;
        }
        case STRING_MEMCELL: {
            VMCall_LibFunc(func->data.string);
            break;
        }
        case TABLE_MEMCELL: {
            VMCall_TableFunctor(func->data.table);
            break;
        }
        default: {
            char* s = VMMemCell_ToString(func);
            ERROR_MSG("VM", "Cannot call non-function type '%s'", s);
            safeFree(&s, "freeing a value-to-string buffer after a call warning");
            executionFinished = 1;
        }
    }
}

// ----------------------------------------------------------


void VMInstruction_ExecutePushArg(VMInstruction* instr){
    VMMemCell_ptr arg = VMInstruction_TranslateOperand(instr->arg1, ax);
    
    assert(arg);

    if(arg->type == UNDEFINED_MEMCELL) {
        ERROR_MSG("VM", "Pushing undefined value as argument");
    }

    VMMemCell_Assign(ExecutionStack[VM_SP], arg);
    VMCall_DecTop();



    totalActuals++;
}

// ----------------------------------------------------------


void VMInstruction_ExecuteFuncEnter(VMInstruction* instr){
    VMMemCell_ptr func = VMInstruction_TranslateOperand(instr->result, ax);
    assert(func);
    assert(VM_PC == vm_program.user_functions[func->data.userfunc].address);

    totalActuals = 0;
    VM_BP = VM_SP;
    VM_SP -= vm_program.user_functions[func->data.userfunc].localsSize;

    // Start every function call with a clean return slot. A function that never
    // executes a `ret` (no return statement, or falls off the end) then yields nil
    // deterministically instead of whatever the previous call happened to leave here.
    VMMemCell_Clear(retval);
    retval->type = NIL_MEMCELL;
}

// ----------------------------------------------------------


void VMInstruction_ExecuteFuncExit(VMInstruction* instr){
    (void)instr;

    unsigned oldSP = VM_SP;
    VM_SP = VMCall_GetEnvValue(VM_BP + VM_SAVEDSP_OFFSET);
    VM_PC = VMCall_GetEnvValue(VM_BP + VM_SAVEDPC_OFFSET);
    VM_BP = VMCall_GetEnvValue(VM_BP + VM_SAVEDBP_OFFSET);

    while(++oldSP <= VM_SP){
        VMMemCell_Clear(ExecutionStack[oldSP]);
    }
}

// ----------------------------------------------------------

void VMInstruction_ExecuteNewTable(VMInstruction* instr){
    VMMemCell_ptr lv = VMInstruction_TranslateOperand(instr->result, NULL);

    assert(lv && ((ExecutionStack[AVM_STACK_SIZE - 1] >= lv && lv > ExecutionStack[VM_SP]) || lv == retval));

    VMMemCell_Clear(lv);

    ax->type = TABLE_MEMCELL;
    ax->data.table = VMTable_New();
    
    VMMemCell_Assign(lv, ax);
    assert(lv->type == TABLE_MEMCELL);
}

// ----------------------------------------------------------

void VMInstruction_ExecuteTableGetElem(VMInstruction* instr){
    VMMemCell_ptr lv = VMInstruction_TranslateOperand(instr->result, NULL);
    VMMemCell_ptr table = VMInstruction_TranslateOperand(instr->arg1, ax);
    VMMemCell_ptr index = VMInstruction_TranslateOperand(instr->arg2, bx);

    assert(lv && ((ExecutionStack[AVM_STACK_SIZE - 1] >= lv && lv > ExecutionStack[VM_SP]) || lv == retval));
    assert(table && (ExecutionStack[AVM_STACK_SIZE - 1] >= table && table > ExecutionStack[VM_SP]));
    assert(index);

    VMMemCell_Clear(lv);
    lv->type = NIL_MEMCELL;

    if(table->type != TABLE_MEMCELL) {
        ERROR_MSG("VM", "Cannot get element from non-table type '%s' at line %d", VMMemCellTypeStrings[table->type], currLine);
    }

    VMMemCell_ptr value = VMTable_Get(table->data.table, index);

    if(value) {
        VMMemCell_Assign(lv, value);
    } else {
        char* indexStr = VMMemCell_ToString(index);
        char* tableStr = VMMemCell_ToString(table);
        ERROR_MSG("VM", "Table '%s' does not have an element with index '%s' at line %d", tableStr, indexStr, currLine);
        safeFree(&indexStr, "freeing index string after missing-element warning");
        safeFree(&tableStr, "freeing table string after missing-element warning");
    }
}

// ----------------------------------------------------------

void VMInstruction_ExecuteTableSetElem(VMInstruction* instr){
    // Compiler emits tablesetelem as (result=table, arg1=index, arg2=value);
    // see quad_Emit(TABLE_SET_ELEM_QUADOP, lvalue, index, value) in the Compiler.
    VMMemCell_ptr table = VMInstruction_TranslateOperand(instr->result, ax);
    VMMemCell_ptr index = VMInstruction_TranslateOperand(instr->arg1, bx);
    VMMemCell_ptr value = VMInstruction_TranslateOperand(instr->arg2, cx);

    assert(table && (ExecutionStack[AVM_STACK_SIZE - 1] >= table && table > ExecutionStack[VM_SP]));
    assert(index && value);

    if(table->type != TABLE_MEMCELL) {
        ERROR_MSG("VM", "Cannot set element in non-table type '%s' at line %d", VMMemCellTypeStrings[table->type], currLine);
    }

    VMTable_Set(table->data.table, index, value);
}

// ----------------------------------------------------------

void VMInstruction_ExecuteJump(VMInstruction* instr){
    assert(instr->result.type == LABEL_IARGTYPE);

    if(!executionFinished) {
        VM_PC = instr->result.val;
    }
}

// ----------------------------------------------------------

void VMInstruction_ExecuteNop(VMInstruction* instr){
    (void)instr; // NOP does nothing, so we ignore the instruction
}

// --------------------------------------------------------------------------------------------------

void VMMemCell_Assign(VMMemCell_ptr lv, VMMemCell_ptr rv){
    assert(lv && rv);

    if(lv == rv) return;
    if(lv->type == TABLE_MEMCELL && rv->type == TABLE_MEMCELL && lv->data.table == rv->data.table) {
        return;
    }

    if(rv->type == UNDEFINED_MEMCELL){
        WARNING_MSG("VM", "Assigning undefined value to memory cell");
    }

    VMMemCell_Clear(lv);

    memcpy(lv, rv, sizeof(struct VMMemCell));

    if(lv->type == STRING_MEMCELL) {
        lv->data.string = safeStrDup(rv->data.string, "duplicating a string for VM value-to-string conversion");
        if (!lv->data.string) {
            fprintf(stderr, "Error: Cannot allocate memory for string in assignment\n");
            exit(EXIT_FAILURE);
        }
    } else if(lv->type == LIBFUNC_MEMCELL) {
        lv->data.libfunc = safeStrDup(rv->data.libfunc, "duplicating a string for VM value-to-string conversion");
        if (!lv->data.libfunc) {
            fprintf(stderr, "Error: Cannot allocate memory for library function in assignment\n");
            exit(EXIT_FAILURE);
        }
    } else if(lv->type == TABLE_MEMCELL) {
        VMTable_IncRefCount(lv->data.table);
    }

}

unsigned totalActuals = 0;

void VMCall_DecTop(){
    if(VM_SP == 0){
        ERROR_MSG("VM", "Stack Overflow at line %d", currLine);
    }
    
    VM_SP--;
}

void VMCall_PushEnvValue(unsigned value){
    ExecutionStack[VM_SP]->type = NUMBER_MEMCELL;
    ExecutionStack[VM_SP]->data.number = (double)value;
    VMCall_DecTop();
}

unsigned VMCall_GetEnvValue(unsigned index){
    assert(ExecutionStack[index]->type == NUMBER_MEMCELL);

    unsigned value = (unsigned) ExecutionStack[index]->data.number;

    assert(ExecutionStack[index]->data.number == (double)value);

    return value;
}

void VMCall_SaveEnvironment(){
    VMCall_PushEnvValue(totalActuals);
    assert(vm_program.instructions[VM_PC].opcode == CALL_IOPCODE);
    VMCall_PushEnvValue(VM_PC + 1);
    VMCall_PushEnvValue(VM_SP + totalActuals + 2);
    VMCall_PushEnvValue(VM_BP);
}

void VMCall_LibFunc(const char* name){
    VMLibraryFunc_ptr func = VMCall_GetLibraryFunc(name);
    if(!func){
        ERROR_MSG("VM", "Library function '%s' not found at line %d", name, currLine);
    }

    VMCall_SaveEnvironment();
    VM_BP = VM_SP;
    totalActuals = 0;

    (*func)();

    if(!executionFinished){
        VMInstruction_ExecuteFuncExit(NULL);
    }
}

void VMCall_PushTableArg(VMTable_ptr table){
    ExecutionStack[VM_SP]->type = TABLE_MEMCELL;
    ExecutionStack[VM_SP]->data.table = table;
    VMTable_IncRefCount(table);
    VMCall_DecTop();
    totalActuals++;
}

void VMCall_TableFunctor(VMTable_ptr table){
    cx->type = STRING_MEMCELL;
    cx->data.string = safeStrDup("()", "duplicating a string for VM value-to-string conversion");

    VMMemCell_ptr func = VMTable_Get(table, cx);

    if(!func){
        ERROR_MSG("VM", "Table does not have a functor at line %d", currLine);
    }

    if(func->type == TABLE_MEMCELL){
        VMCall_TableFunctor(func->data.table);
        return;
    }

    if(func->type == LIBFUNC_MEMCELL){
        VMCall_LibFunc(func->data.libfunc);
        return;
    }

    if(func->type == USERFUNC_MEMCELL){
        VMCall_PushTableArg(table);
        VMCall_SaveEnvironment();
        VM_PC = vm_program.user_functions[func->data.userfunc].address;

        assert(VM_PC < vm_program.instruction_count);
        assert(vm_program.instructions[VM_PC].opcode == FUNCENTER_IOPCODE);
        return;
    }

    ERROR_MSG("VM", "Cannot call non-function type '%s' at line %d", VMMemCell_ToString(func), currLine);
}

unsigned VMCall_TotalActuals(){
    return VMCall_GetEnvValue(VM_BP + VM_NUMACTUALS_OFFSET);
}

VMMemCell_ptr VMCall_GetActual(unsigned index){
    assert(index < VMCall_TotalActuals());

    return ExecutionStack[VM_BP + VM_STACK_ENV_SIZE + index + 1];
}

// --------------------------------------------------------------------------------------------------

char* VMMemCell_ToStringNumber(VMMemCell_ptr cell) {
    char* str = safeCalloc(1, 64, "allocating a number-to-string buffer");

    double number = cell->data.number;
    
    // Check if the number is an integer
    if (number == (int)number) {
        // Format as integer with commas
        int intNum = (int)number;
        char tempStr[32];
        snprintf(tempStr, sizeof(tempStr), "%d", intNum);
        
        // Add commas for readability
        int len = strlen(tempStr);
        int startPos = (intNum < 0) ? 1 : 0;
        int digitCount = len - startPos;
        int commaCount = (digitCount > 3) ? (digitCount - 1) / 3 : 0;
        int newLen = len + commaCount;
        
        str[newLen] = '\0';
        int srcIdx = len - 1;
        int destIdx = newLen - 1;
        int digitsProcessed = 0;
        
        while (srcIdx >= startPos) {
            if (digitsProcessed > 0 && digitsProcessed % 3 == 0) {
                str[destIdx--] = '_';
            }
            str[destIdx--] = tempStr[srcIdx--];
            digitsProcessed++;
        }
        
        // Copy the negative sign if present
        if (startPos == 1) {
            str[0] = '-';
        }
    } else {
        // Format as decimal
        snprintf(str, 64, "%.3f", number);
        
        // Remove trailing zeros after decimal point
        int len = strlen(str);
        while (len > 1 && str[len-1] == '0') {
            str[--len] = '\0';
        }
        if (len > 1 && str[len-1] == '.') {
            str[--len] = '\0';
        }
        
        // Add commas to integer part
        char* dotPos = strchr(str, '.');
        int intPartLen = dotPos ? (dotPos - str) : (long int) strlen(str);
        int startPos = (str[0] == '-') ? 1 : 0;
        int digitCount = intPartLen - startPos;
        int commaCount = (digitCount > 3) ? (digitCount - 1) / 3 : 0;
        
        if (commaCount > 0) {
            char temp[64];
            memset(temp, 0, sizeof(temp)); // Clear the buffer
            
            int srcIdx = intPartLen - 1;
            int destIdx = intPartLen + commaCount - 1;
            int digitsProcessed = 0;
            
            // Copy decimal part first if it exists
            if (dotPos) {
                strcpy(temp + intPartLen + commaCount, dotPos);
            } else {
                temp[intPartLen + commaCount] = '\0'; // Ensure null termination
            }
            
            while (srcIdx >= startPos) {
                if (digitsProcessed > 0 && digitsProcessed % 3 == 0) {
                    temp[destIdx--] = '_';
                }
                temp[destIdx--] = str[srcIdx--];
                digitsProcessed++;
            }
            
            if (startPos == 1) {
                temp[0] = '-';
            }
            
            strcpy(str, temp);
        }
    }
    
    return str;
}

char* VMMemCell_ToStringString(VMMemCell_ptr cell) {
    if (cell->data.string == NULL) {
        return safeStrDup("NULL", "duplicating a string for VM value-to-string conversion");
    }
    return safeStrDup(cell->data.string, "duplicating a string for VM value-to-string conversion");
}

char* VMMemCell_ToStringBool(VMMemCell_ptr cell) {
    return safeStrDup(cell->data.boolean ? "true" : "false", "duplicating a string for VM value-to-string conversion");
}

char* VMMemCell_ToStringTable(VMMemCell_ptr cell) {
    if (cell->data.table == NULL) {
        return safeStrDup("{}", "duplicating a string for VM value-to-string conversion");
    }
    
    VMTable_ptr table = cell->data.table;
    
    // Start with empty table representation
    if (table->total == 0) {
        return safeStrDup("{}", "duplicating a string for VM value-to-string conversion");
    }
    
    // Allocate initial buffer (will grow as needed)
    size_t buffer_size = 256;
    char* result = safeCalloc(1, buffer_size, "allocating a table-to-string buffer");

    strcpy(result, "{ ");
    size_t current_len = 2;
    unsigned elements_printed = 0;
    
    // Iterate through all bucket types
    for (int bucket_type = 0; bucket_type < 5; bucket_type++) {
        VMTableBucket_ptr* bucket_array = table->buckets[bucket_type];
        unsigned bucket_size;
        
        // Determine bucket array size based on type
        if (bucket_type == BOOL_INDEXED_TABLE) {
            bucket_size = 2;
        } else {
            bucket_size = VMTABLE_HASHSIZE;
        }
        
        // Iterate through each bucket in the array
        for (unsigned i = 0; i < bucket_size; i++) {
            VMTableBucket_ptr current = bucket_array[i];
            
            // Iterate through the linked list in this bucket
            while (current != NULL) {
                // Convert key to string
                char* key_str = VMMemCell_ToString(current->key);
                
                // Handle value - check for self-reference
                char* value_str;
                if (current->value->type == TABLE_MEMCELL && 
                    current->value->data.table == table) {
                    value_str = safeStrDup("\033[31mtable::self\033[0m", "duplicating a string for VM value-to-string conversion");
                } else {
                    value_str = VMMemCell_ToString(current->value);
                }
                
                // Calculate needed space for this entry (with color codes)
                size_t entry_len = strlen(key_str) + strlen(value_str) + 30; // Extra space for color codes
                if (elements_printed > 0) entry_len += 2; // for ", "
                
                // Grow buffer if needed
                while (current_len + entry_len + 10 >= buffer_size) {
                    buffer_size *= 2;
                    result = safeRealloc(result, buffer_size, "growing a table-to-string buffer");
                }
                
                // Add separator if not first element
                if (elements_printed > 0) {
                    strcat(result, ", ");
                    current_len += 2;
                }
                
                // Format as "green_key: orange_value" with color codes
                char temp_buffer[1024];
                snprintf(temp_buffer, sizeof(temp_buffer), "\033[32m%s\033[0m: \033[33m%s\033[0m", key_str, value_str);
                strcat(result, temp_buffer);
                current_len += strlen(temp_buffer);
                
                // Clean up
                safeFree(&key_str, "freeing a table entry key string");
                safeFree(&value_str, "freeing a table entry value string");

                elements_printed++;
                current = current->next;
            }
        }
    }
    
    strcat(result, " }");
    
    return result;
}

char* VMMemCell_ToStringUserFunc(VMMemCell_ptr cell) {
    unsigned size = strlen(vm_program.user_functions[cell->data.userfunc].name) + 13; // "UserFunc::" + name + null terminator
    char* str = safeCalloc(1, size, "allocating a userfunc-to-string buffer");
    snprintf(str, size, "UserFunc::%s", vm_program.user_functions[cell->data.userfunc].name);
    return str;
}

char* VMMemCell_ToStringLibFunc(VMMemCell_ptr cell) {
    if (cell->data.libfunc == NULL) {
        return safeStrDup("LibFunc::NULL", "duplicating a string for VM value-to-string conversion");
    }
    unsigned size = strlen(cell->data.libfunc) + 10; // "LibFunc::" + name + null terminator
    char* str = safeCalloc(1, size, "allocating a libfunc-to-string buffer");
    snprintf(str, size, "LibFunc::%s", cell->data.libfunc);
    return str;
}

char* VMMemCell_ToStringNil(VMMemCell_ptr cell) {
    (void)cell;
    return safeStrDup("nil", "duplicating a string for VM value-to-string conversion");
}

char* VMMemCell_ToStringUndefined(VMMemCell_ptr cell) {
    (void)cell;
    return safeStrDup("undefined", "duplicating a string for VM value-to-string conversion");
}

VMToStringFunc_ptr toStringFuncsp[] = {
    VMMemCell_ToStringNumber,
    VMMemCell_ToStringString,
    VMMemCell_ToStringBool,
    VMMemCell_ToStringTable,
    VMMemCell_ToStringUserFunc,
    VMMemCell_ToStringLibFunc,
    VMMemCell_ToStringNil,
    VMMemCell_ToStringUndefined
};

char* VMMemCell_ToString(VMMemCell_ptr cell){
    assert(cell->type >= NUMBER_MEMCELL && cell->type <= UNDEFINED_MEMCELL);
    
    return (*toStringFuncsp[cell->type])(cell);
}


// --------------------------------------------------------------------------------------------------

VMLibraryFunc_ptr VMLibFuncs[] = {
    libFunc_Print,
    libFunc_TypeOf,
    libFunc_TotalArguments,
    libFunc_Argument,
    libFunc_Input,
    libFunc_ObjectMemberKeys,
    libFunc_ObjectTotalMembers,
    libFunc_ObjectCopy,
    libFunc_StrToNum,
    libFunc_MathSqrt,
    libFunc_MathCos,
    libFunc_MathSin
};

char* libFunc_Names[] = {
    "print",
    "typeof",
    "totalarguments",
    "argument",
    "input",
    "objectmemberkeys",
    "objecttotalmembers",
    "objectcopy",
    "strtonum",
    "sqrt",
    "cos",
    "sin"
};

VMLibraryFunc_ptr VMCall_GetLibraryFunc(const char* name){
    for(unsigned i = 0; i < 12; i++){
        if(strcmp(name, libFunc_Names[i]) == 0){
            return VMLibFuncs[i];
        }
    }

    return NULL;
}

void libFunc_Print() {
    unsigned count = VMCall_TotalActuals();

    for(unsigned i = 0; i < count; i++){
        char* str = VMMemCell_ToString(VMCall_GetActual(i));
        printf("%s", str);
        safeFree(&str, "freeing a printed argument string");
    }
}

void libFunc_TypeOf() {
    unsigned count = VMCall_TotalActuals();

    if(count != 1) {
        ERROR_MSG("VM", "typeof expects exactly one argument, got %u at line %d", count, currLine);
    }

    VMMemCell_Clear(retval);
    retval->type = STRING_MEMCELL;
    switch(VMCall_GetActual(0)->type) {
        case NUMBER_MEMCELL:
            retval->data.string = safeStrDup("number", "duplicating a string for VM value-to-string conversion");
            break;
        case STRING_MEMCELL:
            retval->data.string = safeStrDup("string", "duplicating a string for VM value-to-string conversion");
            break;
        case BOOL_MEMCELL:
            retval->data.string = safeStrDup("bool", "duplicating a string for VM value-to-string conversion");
            break;
        case TABLE_MEMCELL:
            retval->data.string = safeStrDup("table", "duplicating a string for VM value-to-string conversion");
            break;
        case USERFUNC_MEMCELL:
            retval->data.string = safeStrDup("userfunc", "duplicating a string for VM value-to-string conversion");
            break;
        case LIBFUNC_MEMCELL:
            retval->data.string = safeStrDup("libfunc", "duplicating a string for VM value-to-string conversion");
            break;
        case NIL_MEMCELL:
            retval->data.string = safeStrDup("nil", "duplicating a string for VM value-to-string conversion");
            break;
        case UNDEFINED_MEMCELL:
            retval->data.string = safeStrDup("undefined", "duplicating a string for VM value-to-string conversion");
            break;
        default:
            ERROR_MSG("VM", "Unknown type at line %d", currLine);
    }
}

void libFunc_TotalArguments(){
    VMMemCell_Clear(retval);

    // The actuals belong to the function that *called* totalarguments, not to
    // totalarguments' own (empty) frame, so step one level up via the saved BP.
    unsigned callerBP = VMCall_GetEnvValue(VM_BP + VM_SAVEDBP_OFFSET);

    if(callerBP == AVM_STACK_SIZE - 1 - globalCount) {
        retval->type = NIL_MEMCELL;
        return;
    }

    retval->type = NUMBER_MEMCELL;
    retval->data.number = (double)VMCall_GetEnvValue(callerBP + VM_NUMACTUALS_OFFSET);
}

void libFunc_Argument(){
    // The index is argument()'s own single actual.
    unsigned count = VMCall_TotalActuals();

    if(count == 0) {
        ERROR_MSG("VM", "argument expects exactly one argument, got %u at line %d", count, currLine);
    }

    VMMemCell_Clear(retval);

    // The actuals being queried belong to the caller of argument(), reached one
    // level up via the saved BP -- not argument()'s own frame.
    unsigned callerBP = VMCall_GetEnvValue(VM_BP + VM_SAVEDBP_OFFSET);

    if(callerBP == AVM_STACK_SIZE - 1 - globalCount) {
        retval->type = NIL_MEMCELL;
        return;
    }

    if(VMCall_GetActual(0)->type != NUMBER_MEMCELL) {
        retval->type = NIL_MEMCELL;
        return;
    }

    unsigned index = (unsigned)VMCall_GetActual(0)->data.number;


    if(index >= VMCall_GetEnvValue(callerBP + VM_NUMACTUALS_OFFSET)) {
        retval->type = NIL_MEMCELL;
        return;
    }

    retval->type = NUMBER_MEMCELL;
    retval->data.number = (double)ExecutionStack[callerBP + VM_STACK_ENV_SIZE + index + 1]->data.number;

}

void libFunc_Input(){
    char buffer[256];
    printf("Input: ");
    if(fgets(buffer, sizeof(buffer), stdin) == NULL) {
        ERROR_MSG("VM", "Failed to read input at line %d", currLine);
    }

    VMMemCell_Clear(retval);

    if(sscanf(buffer, "%lf", &retval->data.number) == 1) {
        retval->type = NUMBER_MEMCELL;
        return;
    }
    
    if(strcmp(buffer, "true\n") == 0 || strcmp(buffer, "true\r\n") == 0) {
        retval->type = BOOL_MEMCELL;
        retval->data.boolean = 1;
        return;
    }
    
    if(strcmp(buffer, "false\n") == 0 || strcmp(buffer, "false\r\n") == 0) {
        retval->type = BOOL_MEMCELL;
        retval->data.boolean = 0;
        return;
    } 
    
    if(strcmp(buffer, "nil\n") == 0 || strcmp(buffer, "nil\r\n") == 0) {
        retval->type = NIL_MEMCELL;
        return;
    }

    retval->type = STRING_MEMCELL;
    retval->data.string = safeStrDup(buffer, "duplicating a string for VM value-to-string conversion");
    if (!retval->data.string) {
        fprintf(stderr, "Error: Cannot allocate memory for input string\n");
        exit(EXIT_FAILURE);
    }
    

    size_t len = strlen(retval->data.string);
    if (len > 0 && retval->data.string[len - 1] == '\n') {
        retval->data.string[len - 1] = '\0';
    }

    if (len > 0 && retval->data.string[len - 1] == '\r') {
        retval->data.string[len - 1] = '\0';
    }

#ifdef DEBUG
    char* echo = VMMemCell_ToString(retval);
    fprintf(stderr, "Input received: %s\n", echo);
    safeFree(&echo, "freeing input-echo debug string");
#endif
}

void libFunc_ObjectMemberKeys(){
    unsigned count = VMCall_TotalActuals();

    if(count == 0) {
        ERROR_MSG("VM", "objectmemberkeys expects one argument but got none");
    }

    VMMemCell_ptr obj = VMCall_GetActual(0);
    VMMemCell_Clear(retval);


    if(obj->type != TABLE_MEMCELL) {
        retval->type = NIL_MEMCELL;
        return;
    }

    // Build the new table in scratch register ax and hand it to retval via
    // VMMemCell_Assign -- the same idiom newtable uses. Assign clears retval's
    // previous value and takes a counted reference, so retval owns the table
    // (refCount 1) and a later clear of retval won't free it from under the caller.
    ax->type = TABLE_MEMCELL;
    ax->data.table = VMTable_New();
    VMMemCell_Assign(retval, ax);

    int memberCount = 0;

    // Seach all buckets of the table in obj
    for(int bucket_type = 0; bucket_type < 5; bucket_type++) {
        VMTableBucket_ptr* bucket_array = obj->data.table->buckets[bucket_type];
        unsigned bucket_size;

        if (bucket_type == BOOL_INDEXED_TABLE) {
            bucket_size = 2;
        } else {
            bucket_size = VMTABLE_HASHSIZE;
        }

        for(unsigned i = 0; i < bucket_size; i++) {
            VMTableBucket_ptr current = bucket_array[i];

            while(current != NULL) {
                VMMemCell_ptr new_key = safeCalloc(1, sizeof(struct VMMemCell), "allocating an objectmemberkeys index key");
                new_key->type = NUMBER_MEMCELL;
                new_key->data.number = (double)memberCount;
                memberCount++;
            
                VMTable_Set(retval->data.table, new_key, current->key);
                          
                current = current->next;
            }
        }
    }


}

void libFunc_ObjectTotalMembers(){
    unsigned count = VMCall_TotalActuals();

    if(count == 0) {
        ERROR_MSG("VM", "objecttotalmembers expects one argument but got none");
    }

    VMMemCell_ptr obj = VMCall_GetActual(0);
    VMMemCell_Clear(retval);

    if(obj->type != TABLE_MEMCELL) {
        retval->type = NIL_MEMCELL;
        return;
    }

    retval->type = NUMBER_MEMCELL;
    retval->data.number = (double)obj->data.table->total;
}

void libFunc_ObjectCopy(){
    unsigned count = VMCall_TotalActuals();

    if(count == 0) {
        ERROR_MSG("VM", "objectcopy expects one argument but got none");
    }

    VMMemCell_ptr obj = VMCall_GetActual(0);
    VMMemCell_Clear(retval);

    if(obj->type != TABLE_MEMCELL) {
        retval->type = NIL_MEMCELL;
        return;
    }

    // Create a shallow copy of the table. Build it in scratch register ax and hand
    // it to retval via VMMemCell_Assign (see objectmemberkeys) so retval owns a
    // counted reference and the copy survives the next clear of retval.
    ax->type = TABLE_MEMCELL;
    ax->data.table = VMTable_New();
    VMMemCell_Assign(retval, ax);

    for(int i = 0; i < 4; i++) {
        retval->data.table->buckets[i] = safeCalloc(VMTABLE_HASHSIZE, sizeof(VMTableBucket_ptr), "allocating an objectcopy table bucket array");
    }
    retval->data.table->buckets[BOOL_INDEXED_TABLE] = safeCalloc(2, sizeof(VMTableBucket_ptr), "allocating an objectcopy bool-indexed bucket array");

    // Copy all elements from the original table to the new table but make shallow copies of the keys and values
    for(int bucket_type = 0; bucket_type < 5; bucket_type++) {
        VMTableBucket_ptr* bucket_array = obj->data.table->buckets[bucket_type];
        unsigned bucket_size;

        if (bucket_type == BOOL_INDEXED_TABLE) {
            bucket_size = 2;
        } else {
            bucket_size = VMTABLE_HASHSIZE;
        }

        for(unsigned i = 0; i < bucket_size; i++) {
            VMTableBucket_ptr current = bucket_array[i];

            while(current != NULL) {
                VMMemCell_ptr new_key = safeCalloc(1, sizeof(struct VMMemCell), "allocating an objectcopy key cell");
                VMMemCell_Assign(new_key, current->key); // Shallow copy of the key

                VMMemCell_ptr new_value = safeCalloc(1, sizeof(struct VMMemCell), "allocating an objectcopy value cell");
                VMMemCell_Assign(new_value, current->value); // Shallow copy of the value

                VMTable_Set(retval->data.table, new_key, new_value);
                          
                current = current->next;
            }
        }
    }

}

void libFunc_StrToNum(){
    unsigned count = VMCall_TotalActuals();

    if(count != 1) {
        ERROR_MSG("VM", "strtonum expects exactly one argument, got %u at line %d", count, currLine);
    }

    VMMemCell_ptr strCell = VMCall_GetActual(0);
    VMMemCell_Clear(retval);

    if(strCell->type != STRING_MEMCELL) {
        retval->type = NIL_MEMCELL;
        return;
    }

    char* endPtr;
    double number = strtod(strCell->data.string, &endPtr);

    if(*endPtr != '\0') {
        retval->type = NIL_MEMCELL;
        return;
    }

    retval->type = NUMBER_MEMCELL;
    retval->data.number = number;
}

// sqrt, cos, sin, consider that argument is in degrees 

void libFunc_MathSqrt() {
    unsigned count = VMCall_TotalActuals();

    if(count == 0) {
        ERROR_MSG("VM", "mathsqrt expects exactly one argument, got %u at line %d", count, currLine);
    }

    VMMemCell_ptr arg = VMCall_GetActual(0);
    VMMemCell_Clear(retval);

    if(arg->type != NUMBER_MEMCELL) {
        retval->type = NIL_MEMCELL;
        return;
    }

    if(arg->data.number < 0) {
        retval->type = NIL_MEMCELL;
        return;
    }

    retval->type = NUMBER_MEMCELL;
    retval->data.number = sqrt(arg->data.number);
}

void libFunc_MathCos() {
    unsigned count = VMCall_TotalActuals();

    if(count == 0) {
        ERROR_MSG("VM", "mathcos expects exactly one argument, got %u at line %d", count, currLine);
    }

    VMMemCell_ptr arg = VMCall_GetActual(0);
    VMMemCell_Clear(retval);

    if(arg->type != NUMBER_MEMCELL) {
        retval->type = NIL_MEMCELL;
        return;
    }

    // Convert degrees to radians
    double radians = arg->data.number * (3.14159265358979323846 / 180.0);
    
    retval->type = NUMBER_MEMCELL;
    retval->data.number = round(cos(radians) * 1000.0) / 1000.0;
}

void libFunc_MathSin() {
    unsigned count = VMCall_TotalActuals();

    if(count == 0) {
        ERROR_MSG("VM", "mathsin expects exactly one argument, got %u at line %d", count, currLine);
    }

    VMMemCell_ptr arg = VMCall_GetActual(0);
    VMMemCell_Clear(retval);

    if(arg->type != NUMBER_MEMCELL) {
        retval->type = NIL_MEMCELL;
        return;
    }

    // Convert degrees to radians
    double radians = arg->data.number * (3.14159265358979323846 / 180.0);
    
    retval->type = NUMBER_MEMCELL;
    retval->data.number = round(sin(radians) * 1000.0) / 1000.0;
}
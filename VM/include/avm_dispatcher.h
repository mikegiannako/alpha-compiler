#ifndef _AVM_DISPATCHER_H_
#define _AVM_DISPATCHER_H_

#include "avm_structs.h"
#include "binary_reader.h"

#define VM_STACK_ENV_SIZE 4
#define VM_NUMACTUALS_OFFSET 4
#define VM_SAVEDPC_OFFSET 3
#define VM_SAVEDSP_OFFSET 2
#define VM_SAVEDBP_OFFSET 1

extern VMMemCell_ptr ax, bx, cx, retval;
extern unsigned VM_SP, VM_PC, VM_BP;
extern unsigned char executionFinished;

VMMemCell_ptr VMInstruction_TranslateOperand(VMArg arg, VMMemCell_ptr reg);

typedef void (*VMExecuteFunc_ptr)(VMInstruction*);

void VMInstruction_ExecuteAssign(VMInstruction* instr);
void VMInstruction_ExecuteArithmetic(VMInstruction* instr);
void VMInstruction_ExecuteIfEq(VMInstruction* instr);
void VMInstruction_ExecuteIfNotEq(VMInstruction* instr);
void VMInstruction_ExecuteComparison(VMInstruction* instr);
void VMInstruction_ExecuteCall(VMInstruction* instr);
void VMInstruction_ExecutePushArg(VMInstruction* instr);
void VMInstruction_ExecuteFuncEnter(VMInstruction* instr);
void VMInstruction_ExecuteFuncExit(VMInstruction* instr);
void VMInstruction_ExecuteNewTable(VMInstruction* instr);
void VMInstruction_ExecuteTableGetElem(VMInstruction* instr);
void VMInstruction_ExecuteTableSetElem(VMInstruction* instr);
void VMInstruction_ExecuteJump(VMInstruction* instr);
void VMInstruction_ExecuteNop(VMInstruction* instr);

// --------------------------------------------------------------------------------------------------

extern unsigned totalActuals;
void VMInstruction_ExecuteCycle(void);
void VMMemCell_Assign(VMMemCell_ptr lv, VMMemCell_ptr rv);
void VMCall_SaveEnvironment();
void VMCall_RestoreEnvironment();
void VMCall_LibFunc(const char* name);
void VMCall_DecTop();
void VMCall_PushTableArg(VMTable_ptr table);
void VMCall_TableFunctor(VMTable_ptr table);
void VMCall_PushEnvValue(unsigned value);
unsigned VMCall_GetEnvValue(unsigned index);
unsigned VMCall_TotalActuals();
VMMemCell_ptr VMCall_GetActual(unsigned index);

// --------------------------------------------------------------------------------------------------


typedef char* (*VMToStringFunc_ptr)(VMMemCell_ptr);
char* VMMemCell_ToString(VMMemCell_ptr cell);

// --------------------------------------------------------------------------------------------------


typedef void (*VMLibraryFunc_ptr)();
VMLibraryFunc_ptr VMCall_GetLibraryFunc(const char* name);
void libFunc_Print();
void libFunc_TypeOf();
void libFunc_TotalArguments();
void libFunc_Argument();
void libFunc_Input();
void libFunc_ObjectMemberKeys();
void libFunc_ObjectTotalMembers();
void libFunc_ObjectCopy();
void libFunc_StrToNum();
void libFunc_MathSqrt();
void libFunc_MathCos();
void libFunc_MathSin();

#endif
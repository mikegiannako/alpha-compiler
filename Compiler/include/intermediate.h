#ifndef _INTERMEDIATE_
#define _INTERMEDIATE_

#include <stdbool.h>

#include "intermediate_rule_types.h"
#include "quad.h"
#include "../build/parser.h"

// ------------------ Scopes/Spaces ------------------

extern unsigned int programVarOffset;
extern unsigned int functionLocalOffset;
extern unsigned int formalArgOffset;
extern unsigned int scopeSpaceCounter;

ScopeSpace_enum currScopeSpace();
unsigned int currScopeOffset();
void incCurScopeOffset();
void enterScopeSpace();
void exitScopeSpace();
void resetFormalArgOffset();
void resetFunctionLocalOffset();
void restoreCurScopeOffset(unsigned int n);

// ------------------ Expr ------------------

void expr_CheckArithm(Expr_ptr expr, const char* context);

void exprArena_FreeAll(void);
void callArena_FreeAll(void);

Expr_ptr expr_FromLvalue(SymbolTableEntry_ptr sym);
Expr_ptr expr_New(ExprType_enum type);
Expr_ptr expr_NewConstString(const char* s);
Expr_ptr expr_NewConstNum(double n);
Expr_ptr expr_NewConstBool(bool b);

Expr_ptr emitIfTableItem(Expr_ptr e);
Expr_ptr memberItem(Expr_ptr lvalue, const char* name);

bool expr_IsBoolConvertible(ExprType_enum type);
bool expr_ToBool(Expr_ptr expr);


// ------------------ Call ------------------

Expr_ptr makeCall(Expr_ptr lvalue, Expr_ptr args);

Call_ptr createCallValue(bool isMethodCall, const char* method_name, Expr_ptr elist);


// ------------------ No-return-value check ------------------
// Best-effort, parse-time diagnostic: warns when the result of a call to a user
// function that has no `return` statement is used as a value. Detection is limited
// to direct calls of statically-known user functions (not calls through variables,
// parameters, anonymous values, methods or functors).

void noReturnCheck_EnterFunc(SymbolTableEntry_ptr func);  // function definition begins
void noReturnCheck_MarkReturn(void);                      // a `return` was seen in the current function
void noReturnCheck_ExitFunc(void);                        // function definition ends
void noReturnCheck_CancelIfCallResult(Expr_ptr expr);     // expr was discarded as a bare statement
void noReturnCheck_ReportAll(void);                       // emit warnings after parsing completes


// ------------------ Temp Var ------------------

extern unsigned int tempVarCounter;

char* newTempVarName();
void resetTempVarCounter();
SymbolTableEntry_ptr newTempVar();
unsigned int isTempName (SymbolTableEntry_ptr sym);
unsigned int expr_IsTemp (Expr_ptr expr);


// ------------------ Short Circuit ------------------

void emitIfNotBool(Expr_ptr* expr);
void emitIfShortCircuitStmt(Expr_ptr* expr);


#endif 
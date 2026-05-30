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
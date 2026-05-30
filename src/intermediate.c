#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "../include/intermediate.h"
#include "../include/memory_operations.h"
#include "../include/error_macros.h"
#include "../include/quad.h"
#include "../include/utils.h"

// ------------------ Scopes/Spaces ------------------

unsigned int programVarOffset = 0;
unsigned int functionLocalOffset = 0;
unsigned int formalArgOffset = 0;
unsigned int scopeSpaceCounter = 1;

ScopeSpace_enum currScopeSpace(){
    if(scopeSpaceCounter == 1) return PROGRAM_VAR_SPACE;
    if(scopeSpaceCounter % 2 == 0) return FORMAL_ARG_SPACE;
    return FUNCTION_LOCAL_SPACE;
}

unsigned int currScopeOffset(){
    switch(currScopeSpace()){
        case PROGRAM_VAR_SPACE: return programVarOffset;
        case FUNCTION_LOCAL_SPACE: return functionLocalOffset;
        case FORMAL_ARG_SPACE: return formalArgOffset;
        default: ERROR_MSG("COMPILER", "currScopeSpace() returned a type not covered by (ScopeSpace) enum");
    }
}

void incCurScopeOffset(){
    switch(currScopeSpace()){
        case PROGRAM_VAR_SPACE: programVarOffset++; break;
        case FUNCTION_LOCAL_SPACE: functionLocalOffset++; break;
        case FORMAL_ARG_SPACE: formalArgOffset++; break;
        default: ERROR_MSG("COMPILER", "currScopeSpace() returned a type not covered by (ScopeSpace) enum");
    }
}

void enterScopeSpace(){
    scopeSpaceCounter++;
}

void exitScopeSpace(){
    assert(scopeSpaceCounter > 1);
    scopeSpaceCounter--;
}

void resetFormalArgOffset(){
    formalArgOffset = 0;
}

void resetFunctionLocalOffset(){
    functionLocalOffset = 0;
}

void restoreCurScopeOffset(unsigned int n){
    switch(currScopeSpace()){
        case PROGRAM_VAR_SPACE: programVarOffset = n; break;
        case FUNCTION_LOCAL_SPACE: functionLocalOffset = n; break;
        case FORMAL_ARG_SPACE: formalArgOffset = n; break;
        default: ERROR_MSG("COMPILER", "currScopeSpace() returned a type not covered by (ScopeSpace) enum");
    }
}


// ------------------ Expr ------------------

void expr_CheckArithm (Expr_ptr expr, const char* context){
    return;
    if ( 
        expr->type == CONST_BOOL_EXPRTYPE   ||
        expr->type == CONST_STRING_EXPRTYPE ||
        expr->type == NIL_EXPRTYPE          ||
        expr->type == NEW_TABLE_EXPRTYPE    ||
        expr->type == PROGRAM_FUNC_EXPRTYPE ||
        expr->type == LIBRARY_FUNC_EXPRTYPE ||
        expr->type == BOOL_EXPR_EXPRTYPE 
    )
    USER_WARNING("COMPILER", "Illegal expr type used in %s", context);
}

Expr_ptr expr_FromLvalue(SymbolTableEntry_ptr sym){
    assert(sym);

    Expr_ptr expr = safeCalloc(1, sizeof(struct Expr), "creating new Expr to wrap SymbolTableEntry");
    expr->sym = sym;

    switch(sym->type){
        case GLOBAL_VAR_SYMTYPE:
        case LOCAL_VAR_SYMTYPE:
        case FORMAL_VAR_SYMTYPE:
            expr->type = VAR_EXPRTYPE;
            break;
        case USER_FUNC_SYMTYPE:
            expr->type = PROGRAM_FUNC_EXPRTYPE;
            break;
        case LIB_FUNC_SYMTYPE:
            expr->type = LIBRARY_FUNC_EXPRTYPE;
            break;
        default:
            ERROR_MSG("COMPILER", "SymbolTableEntry type didn't match any of the (SymbolType) enum cases");
    }

    return expr;
}

Expr_ptr expr_New(ExprType_enum type){
    Expr_ptr expr = safeCalloc(1, sizeof(struct Expr), "creating new Expr from type");
    expr->type = type;
    return expr;
}

Expr_ptr newTempExpr(){
    Expr_ptr expr = safeCalloc(1, sizeof(struct Expr), "creating new Temp Expr");
    expr->type = VAR_EXPRTYPE;
    return expr;
}

Expr_ptr expr_NewConstString(const char* str){
    Expr_ptr expr = expr_New(CONST_STRING_EXPRTYPE);
    expr->strConst = safeStrDup(str, "creating const string expression from string const");
    return expr;
}

Expr_ptr expr_NewConstNum(double num){
    Expr_ptr expr = expr_New(CONST_NUM_EXPRTYPE);
    expr->numConst = num;
    return expr;
}

Expr_ptr expr_NewConstBool(bool b){
    Expr_ptr expr = expr_New(CONST_BOOL_EXPRTYPE);
    expr->boolConst = b;
    return expr;
}

Expr_ptr emitIfTableItem(Expr_ptr expr){
    if(expr->type != TABLE_ITEM_EXPRTYPE) return expr;

    Expr_ptr result = newTempExpr();
    result->sym = expr_IsTemp(expr) ? expr->sym : newTempVar();

    quad_Emit(TABLE_GET_ELEM_QUADOP, result, expr, expr->index, NO_LABEL);

    return result;
}

Expr_ptr memberItem(Expr_ptr lvalue, const char* name){
    lvalue = emitIfTableItem(lvalue);

    Expr_ptr tableItem = expr_New(TABLE_ITEM_EXPRTYPE);
    tableItem->sym = lvalue->sym;
    tableItem->index = expr_NewConstString(name);
    
    return tableItem;
}

bool expr_IsBoolConvertible(ExprType_enum type){
    return ( type == CONST_BOOL_EXPRTYPE   || type == CONST_NUM_EXPRTYPE    || \
             type == CONST_STRING_EXPRTYPE || type == PROGRAM_FUNC_EXPRTYPE || \
             type == LIBRARY_FUNC_EXPRTYPE || type == NEW_TABLE_EXPRTYPE    || \
             type == NIL_EXPRTYPE);
}

bool expr_ToBool(Expr_ptr expr){
    switch(expr->type){
        case CONST_BOOL_EXPRTYPE:   return expr->boolConst;
        case CONST_NUM_EXPRTYPE:    return expr->numConst != 0;
        case CONST_STRING_EXPRTYPE: return expr->strConst[0] != '\0';

        case PROGRAM_FUNC_EXPRTYPE:
        case LIBRARY_FUNC_EXPRTYPE:
        case NEW_TABLE_EXPRTYPE:    return true;

        case NIL_EXPRTYPE:          return false;
        default:
            ERROR_MSG("COMPILER", "Expr_ptr type didn't match any of the (ExprType) enum cases");
    }
}

unsigned int expr_IsTemp (Expr_ptr expr){
    return /*e->type == var_e &&*/ isTempName(expr->sym);
}


// ------------------ Call ------------------

Expr_ptr makeCall(Expr_ptr lvalue, Expr_ptr args){
    Expr_ptr func = emitIfTableItem(lvalue);

    unsigned int totalArgs = 0U;
    while(args){
        quad_Emit(PARAM_QUADOP, NULL, args, NULL, NO_LABEL);
        args = args->next;
        totalArgs++;
    }

    if(lvalue->sym->totalFormals > totalArgs){
        USER_WARNING("COMPILER", "Function %s called with %d arguments, expected %u", lvalue->sym->name, totalArgs, lvalue->sym->totalFormals);
    }

    quad_Emit(CALL_QUADOP, NULL, func, NULL, NO_LABEL);

    Expr_ptr result = expr_New(VAR_EXPRTYPE);
    result->sym = newTempVar();

    quad_Emit(GET_RET_VAL_QUADOP, result, NULL, NULL, NO_LABEL);
    return result;
}

Call_ptr createCallValue(bool isMethodCall, const char* method_name, Expr_ptr elist){
    Call_ptr temp = safeCalloc(1, sizeof(struct Call), "creating Call object");

    temp->elist = elist;
    temp->method_name = method_name ? method_name : NULL;
    temp->isMethodCall = isMethodCall;

    return temp;
}

// ------------------ Temp Var ------------------

unsigned int tempVarCounter = 0U;

char* newTempVarName(){
    char* tempVarName = safeCalloc(countDigits(tempVarCounter) + 1 + 2, sizeof(char), "creating new temp var");
    sprintf(tempVarName, "_t%d", tempVarCounter);
    return tempVarName;
}

void resetTempVarCounter(){
    return;
    tempVarCounter = 0U;
}

SymbolTableEntry_ptr newTempVar(){
    char* tempVarName = newTempVarName();
    SymbolTableEntry_ptr entry = symbolTable_Lookup(tempVarName);

    if(entry == NULL){
        entry = symbolTable_Insert(tempVarName, (scope == 0) ? GLOBAL_VAR_SYMTYPE : LOCAL_VAR_SYMTYPE);
    }

    tempVarCounter++;

    return entry;
}

unsigned int isTempName (SymbolTableEntry_ptr sym){
    return false;
    if(sym == NULL) return false;
    if(strlen(sym->name) < 3) return false;
    return sym->name[0] == '_' && sym->name[1] == 't';
}

// ------------------ Short Circuit ------------------

void emitIfNotBool(Expr_ptr* expr){
    // if(checkBoolean((*expr)->type)){
    //     *expr = newExprConstBool(exprToBoolean(*expr));
    //     return;
    // }

    if((*expr)->type == BOOL_EXPR_EXPRTYPE) return;

    Expr_ptr boolexpr;
    boolexpr = expr_New(BOOL_EXPR_EXPRTYPE);
    boolexpr->trueList = quad_NextLabel();
    quad_Emit(IF_EQ_QUADOP, NULL, *expr, expr_NewConstBool(true), NO_LABEL);

    boolexpr->falseList = quad_NextLabel();
    quad_Emit(JUMP_QUADOP, NULL, NULL, NULL, NO_LABEL);

    *expr = boolexpr;
}

void emitIfShortCircuitStmt(Expr_ptr* expr){
    if((*expr)->type != BOOL_EXPR_EXPRTYPE) return;

    Expr_ptr result = expr_New(BOOL_EXPR_EXPRTYPE);
    result->sym = newTempVar();

    quadIndexList_Patch((*expr)->trueList, quad_NextLabel());
    quad_Emit(ASSIGN_QUADOP, result, expr_NewConstBool(true), NULL, NO_LABEL);

    quad_Emit(JUMP_QUADOP, NULL, NULL, NULL, quad_NextLabel() + 2);

    quadIndexList_Patch((*expr)->falseList, quad_NextLabel());
    quad_Emit(ASSIGN_QUADOP, result, expr_NewConstBool(false), NULL, NO_LABEL);

    *expr = result;
}

#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "../include/intermediate.h"
#include "memory_operations.h"
#include "error_macros.h"
#include "../include/quad.h"
#include "../include/utils.h"

// ------------------ Expr Arena ------------------
// Every Expr is registered here at creation and freed exactly once at the end of
// compilation via exprArena_FreeAll(). Exprs are aliased (the same Expr_ptr is
// stored in multiple quads and in other exprs' `index`/`next`/`elist` fields), so
// they must never be freed through those references. The arena owns them instead:
// one allocation -> one registration -> one free, regardless of aliasing.

typedef struct ExprNode {
    Expr_ptr expr;
    struct ExprNode* next;
}* ExprNode_ptr;

static ExprNode_ptr exprArena = NULL;

static Expr_ptr exprArena_Register(Expr_ptr expr){
    ExprNode_ptr node = safeCalloc(1, sizeof(struct ExprNode), "registering expr in arena");
    node->expr = expr;
    node->next = exprArena;
    exprArena = node;
    return expr;
}

void exprArena_FreeAll(void){
    ExprNode_ptr node = exprArena;
    while(node){
        ExprNode_ptr next = node->next;
        // strConst is the only heap field an Expr owns. sym is borrowed from the
        // symbol table; index/next are other arena Exprs freed as their own nodes.
        if(node->expr->type == CONST_STRING_EXPRTYPE){
            safeFree(&node->expr->strConst, "freeing arena expr const string");
        }
        safeFree(&node->expr, "freeing arena expr");
        safeFree(&node, "freeing arena expr node");
        node = next;
    }
    exprArena = NULL;
}

// ------------------ Call Arena ------------------
// Call structs are grammar bookkeeping consumed mid-parse; same arena treatment
// as exprs. A Call owns nothing: elist is an arena Expr, method_name is a
// lexer-tracked string, isMethodCall is a scalar. So only the struct is freed.

typedef struct CallNode {
    Call_ptr call;
    struct CallNode* next;
}* CallNode_ptr;

static CallNode_ptr callArena = NULL;

static Call_ptr callArena_Register(Call_ptr call){
    CallNode_ptr node = safeCalloc(1, sizeof(struct CallNode), "registering call in arena");
    node->call = call;
    node->next = callArena;
    callArena = node;
    return call;
}

void callArena_FreeAll(void){
    CallNode_ptr node = callArena;
    while(node){
        CallNode_ptr next = node->next;
        safeFree(&node->call, "freeing arena call");
        safeFree(&node, "freeing arena call node");
        node = next;
    }
    callArena = NULL;
}

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
    USER_ERROR("COMPILER", "Illegal expr type used in %s", context);
}

Expr_ptr expr_FromLvalue(SymbolTableEntry_ptr sym){
    assert(sym);

    Expr_ptr expr = exprArena_Register(safeCalloc(1, sizeof(struct Expr), "creating new Expr to wrap SymbolTableEntry"));
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
    Expr_ptr expr = exprArena_Register(safeCalloc(1, sizeof(struct Expr), "creating new Expr from type"));
    expr->type = type;
    return expr;
}

Expr_ptr newTempExpr(){
    Expr_ptr expr = exprArena_Register(safeCalloc(1, sizeof(struct Expr), "creating new Temp Expr"));
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

// ------------------ No-return-value check ------------------
// See intermediate.h. A pending record is created for every direct call to a user
// function; if the call's result expression is later discarded as a bare statement
// the record is cancelled, otherwise (a value use) it is reported at end of parse
// unless the callee turned out to have a return statement.

typedef struct NoReturnPending {
    SymbolTableEntry_ptr callee;
    int                  line;
    bool                 cancelled;
} NoReturnPending;

static NoReturnPending* noReturnPending      = NULL;
static unsigned         noReturnPendingCount = 0;
static unsigned         noReturnPendingCap   = 0;

// User functions currently being defined (innermost on top) so a `return` marks the
// right one. Reuses IntPtrStack via pointer casts to avoid a circular-include for a
// dedicated symbol-pointer stack.
static IntPtrStack_ptr  noReturnFuncStack = NULL;

void noReturnCheck_EnterFunc(SymbolTableEntry_ptr func){
    intPtrStack_Push(&noReturnFuncStack, (int*)func);
}

void noReturnCheck_MarkReturn(void){
    if(!intPtrStack_IsEmpty(noReturnFuncStack)){
        ((SymbolTableEntry_ptr)intPtrStack_Top(noReturnFuncStack))->hasReturn = true;
    }
}

void noReturnCheck_ExitFunc(void){
    if(!intPtrStack_IsEmpty(noReturnFuncStack)){
        intPtrStack_Pop(&noReturnFuncStack);
    }
}

static void noReturnCheck_RegisterCall(SymbolTableEntry_ptr callee, Expr_ptr result){
    if(noReturnPendingCount == noReturnPendingCap){
        noReturnPendingCap = noReturnPendingCap ? noReturnPendingCap * 2 : 16;
        noReturnPending = safeRealloc(noReturnPending, noReturnPendingCap * sizeof(NoReturnPending), "growing the no-return pending list");
    }
    noReturnPending[noReturnPendingCount].callee    = callee;
    noReturnPending[noReturnPendingCount].line      = yylineno;
    noReturnPending[noReturnPendingCount].cancelled = false;
    result->noReturnPendingIdx = ++noReturnPendingCount; // 1-based; 0 means "no record"
}

void noReturnCheck_CancelIfCallResult(Expr_ptr expr){
    if(expr && expr->noReturnPendingIdx){
        noReturnPending[expr->noReturnPendingIdx - 1].cancelled = true;
    }
}

void noReturnCheck_ReportAll(void){
    for(unsigned i = 0; i < noReturnPendingCount; i++){
        NoReturnPending* p = &noReturnPending[i];
        if(!p->cancelled && !p->callee->hasReturn){
            fprintf(stderr,
                ORANGE_TEXT "[SEMANTIC WARNING]" RESET_TEXT
                " - return value of '%s' is used as a value, but '%s' has no return statement"
                BLUE_TEXT " | %s:%d" RESET_TEXT "\n",
                p->callee->name, p->callee->name, sourceFileName, p->line);
        }
    }
    if(noReturnPending) safeFree(&noReturnPending, "freeing the no-return pending list");
    noReturnPendingCount = noReturnPendingCap = 0;
    intPtrStack_Clear(&noReturnFuncStack);
}

Expr_ptr makeCall(Expr_ptr lvalue, Expr_ptr args){
    Expr_ptr func = emitIfTableItem(lvalue);

    unsigned int totalArgs = 0U;
    while(args){
        quad_Emit(PARAM_QUADOP, NULL, args, NULL, NO_LABEL);
        args = args->next;
        totalArgs++;
    }

    if(lvalue->sym->totalFormals > totalArgs){
        USER_ERROR("COMPILER", "Function %s called with %d arguments, expected %u", lvalue->sym->name, totalArgs, lvalue->sym->totalFormals);
    }

    quad_Emit(CALL_QUADOP, NULL, func, NULL, NO_LABEL);

    Expr_ptr result = expr_New(VAR_EXPRTYPE);
    result->sym = newTempVar();

    quad_Emit(GET_RET_VAL_QUADOP, result, NULL, NULL, NO_LABEL);

    // Best-effort: remember this call when it targets a statically-known user
    // function, so using `result` as a value can be flagged later if that function
    // has no return statement. Bare `call;` statements cancel the pending record.
    if(lvalue->sym && lvalue->sym->type == USER_FUNC_SYMTYPE){
        noReturnCheck_RegisterCall(lvalue->sym, result);
    }

    return result;
}

Call_ptr createCallValue(bool isMethodCall, const char* method_name, Expr_ptr elist){
    Call_ptr temp = callArena_Register(safeCalloc(1, sizeof(struct Call), "creating Call object"));

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
    tempVarCounter = 0U;
}

SymbolTableEntry_ptr newTempVar(){
    char* tempVarName = newTempVarName();
    // Temps must resolve within the current scope only. A full symbolTable_Lookup
    // also searches enclosing scopes, so a temp like "_t0" used inside a nested
    // function would reuse the *enclosing* function's "_t0" slot instead of
    // getting its own -- two temps then collapse onto the same local offset and
    // overlap at runtime (e.g. self.data[self.size]).
    SymbolTableEntry_ptr entry = symbolTable_LocalLookup(tempVarName);

    if(entry == NULL){
        entry = symbolTable_Insert(tempVarName, (scope == 0) ? GLOBAL_VAR_SYMTYPE : LOCAL_VAR_SYMTYPE);
    }

    // Both Lookup and Insert copy the name, so the original is no longer needed.
    safeFree(&tempVarName, "freeing temp var name after symbol table copy");

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

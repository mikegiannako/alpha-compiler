#include "../include/rule_handlers.h"
#include "../include/error_macros.h"
#include "../include/symtable.h"
#include "../include/utils.h"
#include "../include/quad.h"

#include <assert.h>

// ------------------ STMT ------------------

void HANDLE_STMT_GENERIC(Stmt_ptr* stmt){
    *stmt = safeCalloc(1, sizeof(struct Stmt), "creating an empty Stmt struct");
}


void HANDLE_STMTLIST(Stmt_ptr* stmt_list, Stmt_ptr parsed_stmts, Stmt_ptr curr_stmt){
    HANDLE_STMT_GENERIC(stmt_list);

    (*stmt_list)->breakList = quadIndexList_Merge(parsed_stmts->breakList, curr_stmt->breakList);
    (*stmt_list)->contList  = quadIndexList_Merge(parsed_stmts->contList, curr_stmt->contList);
    (*stmt_list)->returnList  = quadIndexList_Merge(parsed_stmts->returnList, curr_stmt->returnList);

    safeFree(&parsed_stmts, "freeing duplicate Stmt memory after merging with new upated list");
    safeFree(&curr_stmt,    "freeing duplicate Stmt memory after merging with new upated list");
}


void HANDLE_BREAK(Stmt_ptr* stmt){
    if(loopCounter == 0){
        USER_WARNING("SYNTAX", "Attempted to `break` outside of loop");
    }

    HANDLE_STMT_GENERIC(stmt);
    quad_Emit(JUMP_QUADOP, NULL, NULL, NULL, NO_LABEL);
    (*stmt)->breakList = quadIndexList_New(quad_NextLabel() - 1);
}

void HANDLE_CONTINUE(Stmt_ptr* stmt){
    if(loopCounter == 0){
        USER_WARNING("SYNTAX", "Attempted to `continue` outside of loop");
    }

    HANDLE_STMT_GENERIC(stmt);
    quad_Emit(JUMP_QUADOP, NULL, NULL, NULL, NO_LABEL);
    (*stmt)->contList = quadIndexList_New(quad_NextLabel() - 1);
    
}

void HANDLE_RETURN(Stmt_ptr* stmt, Expr_ptr expr){
    if(uintStack_IsEmpty(functionScopeStack)){
        USER_WARNING("SYNTAX", "Attempted to `return` outside of function");
    }

    if(expr) emitIfShortCircuitStmt(&expr);

    quad_Emit(RET_QUADOP, expr, NULL, NULL, NO_LABEL);

    HANDLE_STMT_GENERIC(stmt);

    (*stmt)->returnList = 0; // quadIndexList_New(quad_NextLabel());
    // TODO: add for phase4
    // quad_Emit(JUMP_QUADOP, NULL, NULL, NULL, NO_LABEL);
}

// ------------------ EXPR ------------------

QuadOp_enum arithmOpToOpcode(enum yytokentype op){
    switch(op){
        case ADD_TOK:   return ADD_QUADOP;
        case MINUS_TOK: return SUB_QUADOP;
        case MUL_TOK:   return MUL_QUADOP;
        case DIV_TOK:   return DIV_QUADOP;
        case MOD_TOK:   return MOD_QUADOP;
        default: ERROR_MSG("COMPILER", "yytokentype arithmetic operator not covered by switch case");
    }
}

QuadOp_enum relOpToOpcode(enum yytokentype op){
    switch(op){
        case EQUAL_TOK:         return IF_EQ_QUADOP;
        case NOT_EQUAL_TOK:     return IF_NOT_EQ_QUADOP;
        case LESS_EQUAL_TOK:    return IF_LESS_EQ_QUADOP;
        case GREATER_EQUAL_TOK: return IF_GREATER_EQ_QUADOP;
        case LESS_TOK:          return IF_LESS_QUADOP;
        case GREATER_TOK:       return IF_GREATER_QUADOP;
        default: ERROR_MSG("COMPILER", "yytokentype relavtive operator not covered by switch case");
    }
}

double arithmOpEval(enum yytokentype op, double left, double right){
    switch(op){
        case ADD_TOK: return left + right;
        case MINUS_TOK: return left - right;
        case MUL_TOK: return left * right;
        case DIV_TOK: 
            if(right == 0){
                USER_WARNING("COMPILER", "division with 0 (when evaluating division with constants)");
                return 0;
            }
            return left / right;
        case MOD_TOK:
            if(right == 0){
                USER_WARNING("COMPILER", "division with 0 (when evaluating division with constants)");
                return 0;
            }
            return (int)left % (int)right;
        default: ERROR_MSG("COMPILER", "yytokentype arithmetic operator not covered by switch case");
    }
}

bool relOpEval(enum yytokentype op, double left, double right){
    switch(op){
        case EQUAL_TOK:         return left == right;
        case NOT_EQUAL_TOK:     return left != right;
        case LESS_EQUAL_TOK:    return left <= right;
        case GREATER_EQUAL_TOK: return left >= right;
        case LESS_TOK:          return left < right;
        case GREATER_TOK:       return left > right;
        default: ERROR_MSG("COMPILER", "yytokentype relavtive operator not covered by switch case");
    }
}


void RULE_EXPR_ARITHOP(Expr_ptr* expr, Expr_ptr left, Expr_ptr right, enum yytokentype op){
    expr_CheckArithm(left, "arithmetic expression");
    expr_CheckArithm(right, "arithmetic expression");

    if(left->type == CONST_NUM_EXPRTYPE && right->type == CONST_NUM_EXPRTYPE){
        *expr = expr_NewConstNum(arithmOpEval(op, left->numConst, right->numConst));
        return;
    }

    if(right->type == CONST_NUM_EXPRTYPE && right->numConst == 0 && op == DIV_TOK){
        USER_WARNING("COMPILER", "division with 0");
    }

    *expr = expr_New(ARITH_EXPR_EXPRTYPE);
    if(expr_IsTemp(left)) (*expr)->sym = left->sym;
    else if(expr_IsTemp(right)) (*expr)->sym = right->sym;
    else (*expr)->sym = newTempVar();

    quad_Emit(arithmOpToOpcode(op), *expr, left, right, NO_LABEL);
}

void RULE_EXPR_RELOP(Expr_ptr* expr, Expr_ptr left, Expr_ptr right, enum yytokentype op){
    expr_CheckArithm(left, "relational expression");
    expr_CheckArithm(right, "relational expression");

    if(left->type == CONST_NUM_EXPRTYPE && right->type == CONST_NUM_EXPRTYPE){
        *expr = expr_NewConstBool(relOpEval(op, left->numConst, right->numConst));
        return;
    }

    *expr = expr_New(BOOL_EXPR_EXPRTYPE);

    (*expr)->trueList = quad_NextLabel();
    quad_Emit(relOpToOpcode(op), NULL, left, right, NO_LABEL);
    
    (*expr)->falseList = quad_NextLabel();
    quad_Emit(JUMP_QUADOP, NULL, NULL, NULL, NO_LABEL);
}

void RULE_EXPR_AND(Expr_ptr* expr, Expr_ptr left, unsigned int marker, Expr_ptr right){
    // TODO: uncomment later probably? constant AND evaluation/replacement
    // if ((left->type == constbool_e && left->boolConst == 0) || (right->type == constbool_e && right->boolConst == 0)){
    //     *expr = newExprConstBool(0);
    //     return;
    // } else if (left->type == constbool_e){ // && left->boolConst == 1
    //     *expr = right;
    //     return;
    // } else if (right->type == constbool_e){ // && right->boolConst == 1 
    //     *expr = left;
    //     return;
    // }

    *expr = expr_New(BOOL_EXPR_EXPRTYPE);

    quadIndexList_Patch(left->trueList, marker);
    (*expr)->trueList = right->trueList;
    (*expr)->falseList = quadIndexList_Merge(left->falseList, right->falseList);
}

void RULE_EXPR_OR(Expr_ptr* expr, Expr_ptr left, unsigned int M, Expr_ptr right){
    // TODO: uncomment later probably? constant OR evaluation/replacement
    // if ((left->type == constbool_e && left->boolConst == 1) || (right->type == constbool_e && right->boolConst == 1)){
    //     *expr = newExprConstBool(1);
    //     return;
    // } else if (left->type == constbool_e){ // && left->boolConst == 0
    //     *expr = right;
    //     return;
    // } else if (right->type == constbool_e){ // && right->boolConst == 0
    //     *expr = left;
    //     return;
    // }

    *expr = expr_New(BOOL_EXPR_EXPRTYPE);

    quadIndexList_Patch(left->falseList, M);
    (*expr)->trueList = quadIndexList_Merge(left->trueList, right->trueList);
    (*expr)->falseList = right->falseList;
}


// ------------------ TERM ------------------

void HANDLE_TERM_UMINUS_EXPR(Expr_ptr* term, Expr_ptr expr){
    expr_CheckArithm(expr, "UMINUS expression");

    emitIfShortCircuitStmt(&expr);

    *term = expr_New(ARITH_EXPR_EXPRTYPE);
    (*term)->sym = expr_IsTemp(expr) ? expr->sym : newTempVar();
    quad_Emit(UMINUS_QUADOP, (*term), expr, NULL, NO_LABEL);
}

void HANDLE_TERM_NOT_EXPR(Expr_ptr* term, Expr_ptr expr){
    if(expr->type == CONST_BOOL_EXPRTYPE){
        *term = expr_NewConstBool(!expr->boolConst);
        return;
    }

    emitIfNotBool(&expr);

    *term = expr_New(BOOL_EXPR_EXPRTYPE);
    (*term)->trueList = expr->falseList;
    (*term)->falseList = expr->trueList;


    (*term)->sym = expr->sym;
}


void check_arithop_lvalue_eligibility(SymbolTableEntry_ptr lvalue, const char* operator){
    if(!lvalue) return;
    
    if(lvalue->type == LIB_FUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to %s Library Function `%s`", operator, lvalue->name);
    }

    if(lvalue->type == USER_FUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to %s User Function `%s` found in scope %d", operator, lvalue->name, lvalue->scope);
    }

}

void HANDLE_TERM_INC_LVAL(Expr_ptr* term, Expr_ptr lvalue){
    check_arithop_lvalue_eligibility(lvalue->sym, "increment");

    expr_CheckArithm(lvalue, "++ lvalue");
    if(lvalue->type == TABLE_ITEM_EXPRTYPE){
        *term = emitIfTableItem(lvalue);
        quad_Emit(ADD_QUADOP, *term, *term, expr_NewConstNum(1), NO_LABEL);
        quad_Emit(TABLE_SET_ELEM_QUADOP, lvalue, lvalue->index, *term, NO_LABEL);
    } else {
        quad_Emit(ADD_QUADOP, lvalue, lvalue, expr_NewConstNum(1), NO_LABEL);
        *term = expr_New(ARITH_EXPR_EXPRTYPE);
        (*term)->sym = newTempVar();
        quad_Emit(ASSIGN_QUADOP, *term, lvalue, NULL, NO_LABEL);
    }
}

void HANDLE_TERM_LVAL_INC(Expr_ptr* term, Expr_ptr lvalue){
    check_arithop_lvalue_eligibility(lvalue->sym, "increment");

    expr_CheckArithm(lvalue, "lvalue ++");
    *term = expr_New(VAR_EXPRTYPE);
    (*term)->sym = newTempVar();
    if(lvalue->type == TABLE_ITEM_EXPRTYPE){
        Expr_ptr val = emitIfTableItem(lvalue);
        quad_Emit(ASSIGN_QUADOP, *term, val, NULL, NO_LABEL);
        quad_Emit(ADD_QUADOP, val, val, expr_NewConstNum(1), NO_LABEL);
        quad_Emit(TABLE_SET_ELEM_QUADOP, lvalue, lvalue->index, val, NO_LABEL);
    } else {
        quad_Emit(ASSIGN_QUADOP, *term, lvalue, NULL, NO_LABEL);
        quad_Emit(ADD_QUADOP, lvalue, lvalue, expr_NewConstNum(1), NO_LABEL);
    }
}

void HANDLE_TERM_DEC_LVAL(Expr_ptr* term, Expr_ptr lvalue){
    check_arithop_lvalue_eligibility(lvalue->sym, "decrement");

    expr_CheckArithm(lvalue, "-- lvalue");
    if(lvalue->type == TABLE_ITEM_EXPRTYPE){
        *term = emitIfTableItem(lvalue);
        quad_Emit(SUB_QUADOP, *term, *term, expr_NewConstNum(1), NO_LABEL);
        quad_Emit(TABLE_SET_ELEM_QUADOP, lvalue, lvalue->index, *term, NO_LABEL);
    } else {
        quad_Emit(SUB_QUADOP, lvalue, lvalue, expr_NewConstNum(1), NO_LABEL);
        *term = expr_New(ARITH_EXPR_EXPRTYPE);
        (*term)->sym = newTempVar();
        quad_Emit(ASSIGN_QUADOP, *term, lvalue, NULL, NO_LABEL);
    }

}

void HANDLE_TERM_LVAL_DEC(Expr_ptr* term, Expr_ptr lvalue){
    check_arithop_lvalue_eligibility(lvalue->sym, "decrement");

    expr_CheckArithm(lvalue, "lvalue --");
    *term = expr_New(VAR_EXPRTYPE);
    (*term)->sym = newTempVar();
    if(lvalue->type == TABLE_ITEM_EXPRTYPE){
        Expr_ptr val = emitIfTableItem(lvalue);
        quad_Emit(ASSIGN_QUADOP, *term, val, NULL, NO_LABEL);
        quad_Emit(SUB_QUADOP, val, val, expr_NewConstNum(1), NO_LABEL);
        quad_Emit(TABLE_SET_ELEM_QUADOP, lvalue, lvalue->index, val, NO_LABEL);
    } else {
        quad_Emit(ASSIGN_QUADOP, *term, lvalue, NULL, NO_LABEL);
        quad_Emit(SUB_QUADOP, lvalue, lvalue, expr_NewConstNum(1), NO_LABEL);
    }
}

// ------------------ ASSIGNEXPR ------------------

void HANDLE_ASSIGNEXPR(Expr_ptr* assignexpr, Expr_ptr lvalue, Expr_ptr expr){
    if(!lvalue) return;

    if(lvalue->sym->type == LIB_FUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to assign value to Library Function '%s'", lvalue->sym->name);
    } else if(lvalue->sym->type == USER_FUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to assign value to User Function '%s'", lvalue->sym->name);
    }

    emitIfShortCircuitStmt(&expr);

    if(lvalue->type == TABLE_ITEM_EXPRTYPE){
        quad_Emit(TABLE_SET_ELEM_QUADOP, lvalue, lvalue->index, expr, NO_LABEL);
        *assignexpr = emitIfTableItem(lvalue);
        (*assignexpr)->type = ASSIGN_EXPR_EXPRTYPE;
    } else {
        quad_Emit(ASSIGN_QUADOP, lvalue, expr, NULL, NO_LABEL);
        *assignexpr = expr_New(ASSIGN_EXPR_EXPRTYPE); // TODO: Maybe replace with newExpr(expr->type)
        (*assignexpr)->sym = newTempVar();
        quad_Emit(ASSIGN_QUADOP, (*assignexpr), lvalue, NULL, NO_LABEL);
    }
}


// ------------------ PRIMARY ------------------

void HANDLE_PRIMARY_FUNCDEF(Expr_ptr* primary, SymbolTableEntry_ptr funcdef){
    *primary = expr_New(PROGRAM_FUNC_EXPRTYPE);
    (*primary)->sym = funcdef;
}

// ------------------ LVALUE ------------------

void HANDLE_LVALUE_ID(Expr_ptr* lvalue, const char* id){
    SymbolTableEntry_ptr entry = symbolTable_Lookup(id);

    if(!entry){
        *lvalue = expr_FromLvalue(symbolTable_Insert(id, (scope == 0 ? GLOBAL_VAR_SYMTYPE : LOCAL_VAR_SYMTYPE)));
        return;
    }

    if(entry->scope != 0 && !uintStack_IsEmpty(functionScopeStack)){
        unsigned int func_scope = uintStack_Top(functionScopeStack);
        if(entry->scope <= func_scope){
            USER_WARNING("SYNTAX", "Symbol `%s` (scope %d) is unreachable as there is at least one active User Function between it and the point of reference (last open function is at scope %d) ", id, entry->scope, func_scope);
        }
    }

    *lvalue = expr_FromLvalue(entry);
}

void HANDLE_LVALUE_LOCAL_ID(Expr_ptr* lvalue, const char* id){
    SymbolTableEntry_ptr entry = symbolTable_LocalLookup(id);

    if(entry) {
        *lvalue = expr_FromLvalue(entry);
        return;
    }

    if(scope != 0){
        entry = symbolTable_GlobalLookup(id);
        if(entry && entry->type == LIB_FUNC_SYMTYPE){
            USER_WARNING("SYNTAX", "`local %s` attempting to overshadow Library Function `%s`", id, id);
            return;
        }
    }

    *lvalue = expr_FromLvalue(symbolTable_Insert(id, (scope == 0 ? GLOBAL_VAR_SYMTYPE : LOCAL_VAR_SYMTYPE)));
}


void HANDLE_LVALUE_GLOBAL_ID(Expr_ptr* lvalue, const char* id){
    SymbolTableEntry_ptr entry = symbolTable_GlobalLookup(id);

    if(!entry){
        USER_WARNING("SYNTAX", "Symbol `%s` does not exist in global scope", id);
    }

    *lvalue = expr_FromLvalue(entry);
}

// ------------------ MEMBER ------------------

void HANDLE_MEMBER_DOT(Expr_ptr* member, Expr_ptr lvalue, const char* id){
    if(!lvalue) return;

    if(lvalue->sym->type == LIB_FUNC_SYMTYPE || lvalue->sym->type == USER_FUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to get key '%s' on function '%s' instead of an object", id, lvalue->sym->name);
    }

    *member = memberItem(lvalue, id);
}

void HANDLE_MEMBER_BRACKET(Expr_ptr* member, Expr_ptr lvalue, Expr_ptr expr){
    if(!lvalue) return;

    if(lvalue->sym->type == LIB_FUNC_SYMTYPE || lvalue->sym->type == USER_FUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to get member value on function '%s' instead of an object", lvalue->sym->name);
    }

    emitIfShortCircuitStmt(&expr);

    lvalue = emitIfTableItem(lvalue);
    *member = expr_New(TABLE_ITEM_EXPRTYPE);
    (*member)->sym = lvalue->sym;
    (*member)->index = expr;
}

// ------------------ CALL ------------------

void HANDLE_CALL_LVALUE_CALLSUFFIX(Expr_ptr* call, Expr_ptr lvalue, Call_ptr callsuffix){
    assert(callsuffix);
    if(!lvalue) return;

    if((lvalue->sym->type == LIB_FUNC_SYMTYPE || lvalue->sym->type == USER_FUNC_SYMTYPE) && callsuffix->isMethodCall){
        USER_WARNING("SYNTAX", "Attempted to call method '%s' on function '%s' instead of an object", callsuffix->method_name, lvalue->sym->name);
    }

    lvalue = emitIfTableItem(lvalue);
    if(callsuffix->isMethodCall){
        Expr_ptr temp = lvalue;
        lvalue = emitIfTableItem(memberItem(temp, callsuffix->method_name));
        
        Expr_ptr temp_list = callsuffix->elist;
        if(temp_list == NULL) callsuffix->elist = temp;
        else {
            while(temp_list->next) temp_list = temp_list->next;
            temp_list->next = temp;
        }
    }
    *call = makeCall(lvalue, callsuffix->elist);
}

void HANDLE_CALL_FUNCDEF_ELIST(Expr_ptr* call, SymbolTableEntry_ptr funcdef, Expr_ptr elist){
    Expr_ptr func = expr_New(PROGRAM_FUNC_EXPRTYPE);
    func->sym = funcdef;
    *call = makeCall(func, elist);
}


// ------------------ OBJECTDEF ------------------

void HANDLE_OBJECTDEF_ELIST(Expr_ptr* objectdef, Expr_ptr elist){
    Expr_ptr object = expr_New(NEW_TABLE_EXPRTYPE);
    object->sym = newTempVar();
    quad_Emit(TABLE_CREATE_QUADOP, object, NULL, NULL, NO_LABEL);
    unsigned int totalCount = 0;
    for(Expr_ptr temp = elist; temp; temp = temp->next) totalCount++;

    for(int i = 0; elist; elist = elist->next){
        quad_Emit(TABLE_SET_ELEM_QUADOP, object, expr_NewConstNum(totalCount - 1 - i++), elist, NO_LABEL);
    }

    *objectdef = object;
}

void HANDLE_OBEJCTDEF_INDEXED(Expr_ptr* objectdef, Expr_ptr indexed){
    Expr_ptr object = expr_New(NEW_TABLE_EXPRTYPE);
    object->sym = newTempVar();
    quad_Emit(TABLE_CREATE_QUADOP, object, NULL, NULL, NO_LABEL);
    while(indexed){
        quad_Emit(TABLE_SET_ELEM_QUADOP, object, indexed->index, indexed, NO_LABEL);
        indexed = indexed->next;
    }
    *objectdef = object;
}


// ------------------ FUNCDEF ------------------

static unsigned int anonFuncCounter = 0;

void RULE_FUNCDEF(SymbolTableEntry_ptr* funcdef, SymbolTableEntry_ptr funcdeclare, unsigned int funcparams, unsigned int funcbody){
    // Exiting Formal Arg ScopeSpace to return to whatever scope space we were in before declaring the function
    exitScopeSpace();
    // Restoring the scope offset (we had to reset it as all functions have their own locals starting form index 0);
    unsigned int offset = uintStack_Pop(&scopeOffsetStack);
    restoreCurScopeOffset(offset);

    funcdeclare->totalLocals = funcbody;
    funcdeclare->totalFormals = funcparams;
    *funcdef = funcdeclare;

    quad_Emit(FUNC_END_QUADOP, expr_FromLvalue(*funcdef), NULL, NULL, NO_LABEL);


    // TODO: Uncomment for phase4
    // Pathcing the jump before funcstart label so that during serial execution the function code is skipped
    // unsigned int label = uintStack_Pop(&funcstartJumpStack);
    // quad_PatchLabel(label, quad_NextLabel());
}

void HANDLE_FUNCNAME_ID(SymbolTableEntry_ptr *funcname, const char* id){
    assert(id);

    SymbolTableEntry_ptr entry = symbolTable_Lookup(id);

    if(!entry){
        *funcname = symbolTable_Insert(id, USER_FUNC_SYMTYPE);
        return;
    }

    if(entry->type == USER_FUNC_SYMTYPE && entry->scope == scope){
        USER_WARNING("SYNTAX", "User Function `%s` has already been declared in the same scope", id);
        return;
    } else if((entry->type == LOCAL_VAR_SYMTYPE || entry->type == GLOBAL_VAR_SYMTYPE || entry->type == FORMAL_VAR_SYMTYPE) && entry->scope == scope){
        USER_WARNING("SYNTAX", "Attempted to redeclare symbol `%s` as a User Function (already exists as Variable in the same scope)", id);
        return;
    } else if(entry->type == LIB_FUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "User Function '%s' declaration attempting to overshadow Library Function", id);
        return;
    }

    *funcname = symbolTable_Insert(id, USER_FUNC_SYMTYPE); 
    
}

void HANDLE_FUNCNAME_ANON(SymbolTableEntry_ptr *funcname){
    char* anonFuncName = safeCalloc(1, 2 + countDigits(anonFuncCounter) + 1, "allocating memory for anonymous function name");
    sprintf(anonFuncName, "_f%u", anonFuncCounter);
    *funcname = symbolTable_Insert(anonFuncName, USER_FUNC_SYMTYPE);
    safeFree(&anonFuncName, "freeing memory of temp anonymous function name");
    ++anonFuncCounter;
    return;
}

void HANDLE_FUNCDECLARE_FUNCNAME(SymbolTableEntry_ptr *funcdecl, SymbolTableEntry_ptr name_sym){
    // TODO: Uncomment for phase4
    // uintStack_Push(&funcstartJumpStack, quad_NextLabel());
    // quad_Emit(JUMP_QUADOP, NULL, NULL, NULL, NO_LABEL);

    name_sym->iaddress = quad_NextLabel();
    *funcdecl = name_sym;
    quad_Emit(FUNC_START_QUADOP, expr_FromLvalue(*funcdecl), NULL, NULL, NO_LABEL);
    
    // Keeping the current scope offset as we'll need to reset it for the formal arguments
    uintStack_Push(&scopeOffsetStack, currScopeOffset());
    // Entering formal args space
    enterScopeSpace();
    assert(currScopeSpace() == FORMAL_ARG_SPACE);
    resetFormalArgOffset();
}

void HANDLE_FUNCPARAMS(unsigned int* funcparams, unsigned int idlist){
    *funcparams = idlist;

    // Keeping the temp var count so that it can be restored later to keep counting. This is done because
    // each function has each own space so temp variable start from 0.
    uintStack_Push(&tempVarCountStack, tempVarCounter);
    resetTempVarCounter();

    // Entering function local space
    enterScopeSpace();
    assert(currScopeSpace() == FUNCTION_LOCAL_SPACE);
    resetFunctionLocalOffset();
}

void HANDLE_FUNCBODY(unsigned int* funcbody, Stmt_ptr block){
    *funcbody = currScopeOffset();
    exitScopeSpace();
    assert(currScopeSpace() == FORMAL_ARG_SPACE);

    // Restoring temp var count to continue counting from where we left before func declare
    tempVarCounter = uintStack_Pop(&tempVarCountStack);
    // TODO: Uncomment for phase4
    // quadIndexList_Patch(block->returnList, quad_NextLabel());
}

// ------------------ CONST ------------------

// ------------------ IDLIST ------------------

void HANDLE_IDLIST(unsigned int *idlist, const char* id, unsigned int list_tail){
    assert(id);

    SymbolTableEntry_ptr entry = symbolTable_Lookup(id);

    if(entry){
        /* if(entry->type == USER_FUNC_SYMTYPE){
            USER_WARNING("SYNTAX", "Attempted to redeclare symbol `%s` as a Formal Argument (already exists as active User Function in scope %d)", id, entry->scope);
            return;
        } else */if(entry->type == LIB_FUNC_SYMTYPE){
            USER_WARNING("SYNTAX", "Formal Argument '%s' declaration attempting to overshadow Library Function", id);
            return;
        } else if(entry->type == FORMAL_VAR_SYMTYPE && entry->scope == scope){
            USER_WARNING("SYNTAX", "Attempted to redeclare Formal Argument '%s' (already exists in the same function declaration)", id);
            return;
        }
    }

    (void) symbolTable_Insert(id, FORMAL_VAR_SYMTYPE);

    *idlist = list_tail + 1;
}

// ------------------ IF ------------------

void HANDLE_IFSTMT_IF_ELSE_STMT(Stmt_ptr* ifstmt, unsigned int ifprefix, Stmt_ptr stmt1, unsigned int elseprefix, Stmt_ptr stmt2){
    assert(stmt1);
    assert(stmt2);
    
    quad_PatchLabel(ifprefix, elseprefix + 1);
    quad_PatchLabel(elseprefix, quad_NextLabel());

    HANDLE_STMT_GENERIC(ifstmt);

    (*ifstmt)->breakList = quadIndexList_Merge(stmt1->breakList, stmt2->breakList);
    (*ifstmt)->contList  = quadIndexList_Merge(stmt1->contList, stmt2->contList);
    (*ifstmt)->returnList = quadIndexList_Merge(stmt1->returnList, stmt2->returnList);
}

void HANDLE_IFSTMT_IFPREFIX_STMT(Stmt_ptr* ifstmt, unsigned int ifprefix, Stmt_ptr stmt){
    quad_PatchLabel(ifprefix, quad_NextLabel());

    *ifstmt = stmt;
}

void HANDLE_IFPREFIX_IF_EXPR(unsigned int* ifprefix, Expr_ptr expr){
    emitIfShortCircuitStmt(&expr);

    quad_Emit(IF_EQ_QUADOP, NULL, expr, expr_NewConstBool(true), quad_NextLabel() + 2);
    *ifprefix = quad_NextLabel();
    quad_Emit(JUMP_QUADOP, NULL, NULL, NULL, NO_LABEL);
}

void HANDLE_ELSEPREFIX_ELSE(unsigned int* elseprefix){
    *elseprefix = quad_NextLabel();
    quad_Emit(JUMP_QUADOP, NULL, NULL, NULL, NO_LABEL);
}

// ------------------ WHILE ------------------

void HANDLE_WHILESTMT(Stmt_ptr* whilestmt, unsigned int whilestart, unsigned int whileexpr, Stmt_ptr stmt){
    quad_Emit(JUMP_QUADOP, NULL, NULL, NULL, whilestart);
    quad_PatchLabel(whileexpr, quad_NextLabel());

    quadIndexList_Patch(stmt->breakList, quad_NextLabel());
    quadIndexList_Patch(stmt->contList, whilestart);

    stmt->breakList = 0U;
    stmt->contList = 0U;
    *whilestmt = stmt;
}

void HANDLE_WHILEEXPR(unsigned int* whileexpr, Expr_ptr expr){
    emitIfShortCircuitStmt(&expr);

    quad_Emit(IF_EQ_QUADOP, NULL, expr, expr_NewConstBool(true), quad_NextLabel() + 2);
    *whileexpr = quad_NextLabel();
    quad_Emit(JUMP_QUADOP, NULL, NULL, NULL, NO_LABEL);
}

// ------------------ FOR ------------------

void RULE_FORSTMT_FORPREFIX_ELIST_STMT(Stmt_ptr* forstmt, ForLoopPrefix_ptr forprefix, unsigned int jump1, \
                                            Expr_ptr elist, unsigned int jump2, Stmt_ptr stmt, unsigned int jump3)
{
    quad_PatchLabel(forprefix->enter, jump2 + 1);
    quad_PatchLabel(jump1, quad_NextLabel());
    quad_PatchLabel(jump2, forprefix->test);
    quad_PatchLabel(jump3, jump1 + 1);

    quadIndexList_Patch(stmt->breakList, quad_NextLabel());
    quadIndexList_Patch(stmt->contList, jump1 + 1);

    stmt->breakList = 0U;
    stmt->contList = 0U;
    *forstmt = stmt;
}



void RULE_FORPREFIX_FOR_ELIST_EXPR(ForLoopPrefix_ptr* forprefix, Expr_ptr elist, unsigned int marker, Expr_ptr expr){
    emitIfShortCircuitStmt(&expr);

    *forprefix = safeCalloc(1, sizeof(struct ForLoopPrefix), "creating new ForLoopPrefix struct for forprefix rule");
    (*forprefix)->test = marker;
    (*forprefix)->enter = quad_NextLabel();

    quad_Emit(IF_EQ_QUADOP, NULL, expr, expr_NewConstBool(true), NO_LABEL);
}
#include "../include/rule_handlers.h"
#include "../include/error_macros.h"
#include "../include/symtable.h"
#include "../include/utils.h"

#include <assert.h>


void HANDLE_BREAK(){
    if(loopCounter == 0){
        USER_WARNING("SYNTAX", "Attempted to `break` outside of loop - line %d", yylineno);
    }
}

void HANDLE_CONTINUE(){
    if(loopCounter == 0){
        USER_WARNING("SYNTAX", "Attempted to `continue` outside of loop - line %d", yylineno);
    }
}

void HANDLE_RETURN(){
    if(uintStack_IsEmpty(functionScopeStack)){
        USER_WARNING("SYNTAX", "Attempted to `return` outside of function - line %d", yylineno);
    }
}

//========================

void check_arithop_lvalue_eligibility(symbolTableEntry_ptr lvalue, const char* operator){
    if(lvalue->type == LIBFUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to %s Library Function `%s` - line %d", operator, lvalue->name, yylineno);
    }

    if(lvalue->type == USERFUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to %s User Function `%s` found in scope %d - line %d", operator, lvalue->name, lvalue->scope, yylineno);
    }

}

void HANDLE_TERM_INC_LVAL(symbolTableEntry_ptr lvalue){
    check_arithop_lvalue_eligibility(lvalue, "increment");
}

void HANDLE_TERM_LVAL_INC(symbolTableEntry_ptr lvalue){
    check_arithop_lvalue_eligibility(lvalue, "increment");
}

void HANDLE_TERM_DEC_LVAL(symbolTableEntry_ptr lvalue){
    check_arithop_lvalue_eligibility(lvalue, "decrement");

}

void HANDLE_TERM_LVAL_DEC(symbolTableEntry_ptr lvalue){
    check_arithop_lvalue_eligibility(lvalue, "decrement");
}


//========================

void HANDLE_LVALUE_ID(symbolTableEntry_ptr *lvalue, const char* id){
    symbolTableEntry_ptr entry = symbolTable_Lookup(id);

    if(!entry){
        *lvalue = symbolTable_Insert(id, (scope == 0 ? GLOBALVAR_SYMTYPE : LOCALVAR_SYMTYPE));
        return;
    }

    if(entry->scope != 0 && !uintStack_IsEmpty(functionScopeStack)){
        unsigned int func_scope = uintStack_Top(functionScopeStack);
        if(entry->scope <= func_scope){
            USER_WARNING("SYNTAX", "Symbol `%s` (scope %d) cannot be reached within last open function's block (scope %d) - line %d", id, entry->scope, func_scope, yylineno);
        }
    }
    
    
    *lvalue = entry;
}

void HANDLE_LVALUE_LOCAL_ID(symbolTableEntry_ptr *lvalue, const char* id){
    *lvalue = symbolTable_LocalLookup(id);

    if(*lvalue) return;

    if(scope != 0){
        *lvalue = symbolTable_GlobalLookup(id);
        if((*lvalue) && (*lvalue)->type == LIBFUNC_SYMTYPE){
            USER_WARNING("SYNTAX", "`local %s` attempting to overshadow Library Function `%s` - line %d", id, id, yylineno);
            return;
        }
    }

    symbolTable_Insert(id, (scope == 0 ? GLOBALVAR_SYMTYPE : LOCALVAR_SYMTYPE));
}


void HANDLE_LVALUE_GLOBAL_ID(symbolTableEntry_ptr *lvalue, const char* id){
    *lvalue = symbolTable_GlobalLookup(id);

    if(!*lvalue){
        USER_WARNING("SYNTAX", "Symbol `%s` does not exist in global scope - line %d", id, yylineno);
    }
}

//========================

static unsigned int anonFuncCounter = 0;

void HANDLE_FUNCDECLARE_ID(symbolTableEntry_ptr *lvalue, const char* id){
    assert(id);

    symbolTableEntry_ptr entry = symbolTable_Lookup(id);

    if(!entry){
        *lvalue = symbolTable_Insert(id, USERFUNC_SYMTYPE);
        return;
    }

    if(entry->type == USERFUNC_SYMTYPE && entry->scope == scope){
        USER_WARNING("SYNTAX", "User Function `%s` has already been declared in the same scope - line %d", id, yylineno);
    } else if((entry->type == LOCALVAR_SYMTYPE || entry->type == GLOBALVAR_SYMTYPE) && entry->scope == scope){
        USER_WARNING("SYNTAX", "Attempted to redeclare symbol `%s` as a User Function (already exists as Variable in the same scope) - line %d", id, yylineno);
    } else if(entry->type == LIBFUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to overshadow Library Function `%s` - line %d", id, yylineno);
    }

    *lvalue = symbolTable_Insert(id, USERFUNC_SYMTYPE);
}

void HANDLE_FUNCDECLARE_ANON(symbolTableEntry_ptr *lvalue){
    char* anonFuncName = safeCalloc(1, 2 + countDigits(anonFuncCounter) + 1, "allocating memory for anonymous function name");
    sprintf(anonFuncName, "_f%u", anonFuncCounter);
    *lvalue = symbolTable_Insert(anonFuncName, USERFUNC_SYMTYPE);
    safeFree(&anonFuncName, "freeing memory of temp anonymous function name");
    ++anonFuncCounter;
    return;
}
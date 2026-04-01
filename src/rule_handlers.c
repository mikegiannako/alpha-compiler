#include "../include/rule_handlers.h"
#include "../include/error_macros.h"
#include "../include/symtable.h"
#include "../include/utils.h"

#include <assert.h>


void HANDLE_BREAK(){
    if(loopCounter == 0){
        USER_WARNING("SYNTAX", "Attempted to `break` outside of loop");
    }
}

void HANDLE_CONTINUE(){
    if(loopCounter == 0){
        USER_WARNING("SYNTAX", "Attempted to `continue` outside of loop");
    }
}

void HANDLE_RETURN(){
    if(uintStack_IsEmpty(functionScopeStack)){
        USER_WARNING("SYNTAX", "Attempted to `return` outside of function");
    }
}

//========================

void check_arithop_lvalue_eligibility(symbolTableEntry_ptr lvalue, const char* operator){
    if(!lvalue) return;
    
    if(lvalue->type == LIBFUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to %s Library Function `%s`", operator, lvalue->name);
    }

    if(lvalue->type == USERFUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to %s User Function `%s` found in scope %d", operator, lvalue->name, lvalue->scope);
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

void HANDLE_ASSIGNEXPR(symbolTableEntry_ptr lvalue){
    if(!lvalue) return;

    if(lvalue->type == LIBFUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to assign value to Library Function '%s'", lvalue->name);
    } else if(lvalue->type == USERFUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to assign value to User Function '%s'", lvalue->name);
    }
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
            USER_WARNING("SYNTAX", "Symbol `%s` (scope %d) is unreachable as there is at least one active User Function between it and the point of reference (last open function is at scope %d) ", id, entry->scope, func_scope);
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
            USER_WARNING("SYNTAX", "`local %s` attempting to overshadow Library Function `%s`", id, id);
            return;
        }
    }

    symbolTable_Insert(id, (scope == 0 ? GLOBALVAR_SYMTYPE : LOCALVAR_SYMTYPE));
}


void HANDLE_LVALUE_GLOBAL_ID(symbolTableEntry_ptr *lvalue, const char* id){
    *lvalue = symbolTable_GlobalLookup(id);

    if(!*lvalue){
        USER_WARNING("SYNTAX", "Symbol `%s` does not exist in global scope", id);
    }
}

//========================

void HANDLE_MEMBER_LVALUE_ID(symbolTableEntry_ptr lvalue, const char* id){
    if(!lvalue) return;

    if(lvalue->type == LIBFUNC_SYMTYPE || lvalue->type == USERFUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to get key '%s' on function '%s' instead of an object", id, lvalue->name);
    }
}

void HANDLE_MEMBER_LVALUE_EXPR(symbolTableEntry_ptr lvalue){
    if(!lvalue) return;

    if(lvalue->type == LIBFUNC_SYMTYPE || lvalue->type == USERFUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to get member value on function '%s' instead of an object", lvalue->name);
    }
}

//========================

void HANDLE_CALL_LVALUE_CALLSUFFIX(symbolTableEntry_ptr lvalue, Call_ptr callsuffix){
    assert(callsuffix);
    if(!lvalue) return;

    if((lvalue->type == LIBFUNC_SYMTYPE || lvalue->type == USERFUNC_SYMTYPE) && callsuffix->isMethodCall){
        USER_WARNING("SYNTAX", "Attempted to call method '%s' on function '%s' instead of an object", callsuffix->method_name, lvalue->name);
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
        USER_WARNING("SYNTAX", "User Function `%s` has already been declared in the same scope", id);
    } else if((entry->type == LOCALVAR_SYMTYPE || entry->type == GLOBALVAR_SYMTYPE || entry->type == FORMALVAR_SYMTYPE) && entry->scope == scope){
        USER_WARNING("SYNTAX", "Attempted to redeclare symbol `%s` as a User Function (already exists as Variable in the same scope)", id);
    } else if(entry->type == LIBFUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "User Function '%s' declaration attempting to overshadow Library Function", id);
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

//========================

void HANDLE_IDLIST(symbolTableEntry_ptr *idlist, const char* id, symbolTableEntry_ptr list_tail){
    assert(id);

    symbolTableEntry_ptr entry = symbolTable_Lookup(id);

    if(entry){
        if(entry->type == USERFUNC_SYMTYPE){
            USER_WARNING("SYNTAX", "Attempted to redeclare symbol `%s` as a Formal Argument (already exists as active User Function in scope %d)", id, entry->scope);
            return;
        } else if(entry->type == LIBFUNC_SYMTYPE){
            USER_WARNING("SYNTAX", "Formal Argument '%s' declaration attempting to overshadow Library Function", id);
            return;
        } else if(entry->type == FORMALVAR_SYMTYPE && entry->scope == scope){
            USER_WARNING("SYNTAX", "Attempted to redeclare Formal Argument '%s' (already exists in the same function declaration)", id);
        }
    }

    *idlist = symbolTable_Insert(id, FORMALVAR_SYMTYPE);
    // TODO: Link the formal args via tail when changed to Expressions instead of symbols
}
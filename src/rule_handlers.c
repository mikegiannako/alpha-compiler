#include "../include/rule_handlers.h"
#include "../include/error_macros.h"
#include "../include/symtable.h"


void HANDLE_BREAK(){
    if(loopCounter == 0){
        USER_WARNING("SYNTAX", "Attempted to `break` while not inside loop - line %d", yylineno);
    }
}

void HANDLE_CONTINUE(){
    if(loopCounter == 0){
        USER_WARNING("SYNTAX", "Attempted to `continue` while not inside loop - line %d", yylineno);
    }
}

void HANDLE_RETURN(){
    if(uintStack_IsEmpty(functionScopeStack)){
        USER_WARNING("SYNTAX", "Attempted to `return` while not inside function - line %d", yylineno);
    }
}

//========================

void check_arithop_lvalue_eligibility(symbolTableEntry_ptr lvalue, const char* operator){
    if(lvalue->type == LIBFUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to %s library function `%s` - line %d", operator, lvalue->name, yylineno);
    }

    if(lvalue->type == USERFUNC_SYMTYPE){
        USER_WARNING("SYNTAX", "Attempted to %s user function `%s` found in scope %d - line %d", operator, lvalue->name, lvalue->scope, yylineno);
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
        symbolTable_Insert(id, (scope == 0 ? GLOBALVAR_SYMTYPE : LOCALVAR_SYMTYPE));
    } else {
        if(entry->scope != 0 && !uintStack_IsEmpty(functionScopeStack)){
            unsigned int func_scope = uintStack_Top(functionScopeStack);
            if(entry->scope <= func_scope){
                USER_WARNING("SYNTAX", "Symbol `%s` (scope %d) cannot be reached within last open function's block (scope %d) - line %d", id, entry->scope, func_scope, yylineno);
            }
        }
    }
    
    *lvalue = entry;
}

void HANDLE_LVALUE_LOCAL_ID(symbolTableEntry_ptr *lvalue, const char* id){
    symbolTableEntry_ptr entry = symbolTable_LocalLookup(id);

    if(entry){
        *lvalue = entry;
        return;
    }

    if(scope != 0){
        entry = symbolTable_GlobalLookup(id);
        if(entry && entry->type == LIBFUNC_SYMTYPE){
            USER_WARNING("SYNTAX", "`local %s` attempting to overshadow library function `%s` - line %d", id, id, yylineno);
            return;
        }
    }

    symbolTable_Insert(id, (scope == 0 ? GLOBALVAR_SYMTYPE : LOCALVAR_SYMTYPE));
    
    *lvalue = entry;
}


void HANDLE_LVALUE_GLOBAL_ID(symbolTableEntry_ptr *lvalue, const char* id){
    symbolTableEntry_ptr entry = symbolTable_GlobalLookup(id);

    if(!entry){
        USER_WARNING("SYNTAX", "Symbol `%s` does not exist in global scope - line %d", id, yylineno);
    }

    *lvalue = entry;
}
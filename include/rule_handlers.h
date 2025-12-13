#ifndef _RULE_HANDLERS_H_
#define _RULE_HANDLERS_H_

#include "../include/symtable.h"
#include "../include/generic_stack.h"

extern unsigned int loopCounter;
extern unsigned int scope;
extern int yylineno;
extern uintStack_ptr functionScopeStack;
extern uintStack_ptr loopCounterStack;
extern uintStack_ptr funcstartJumpStack;
extern uintStack_ptr tempVarCountStack;

void HANDLE_BREAK();
void HANDLE_CONTINUE();
void HANDLE_RETURN();

//========================

void HANDLE_TERM_INC_LVAL(symbolTableEntry_ptr lvalue);
void HANDLE_TERM_LVAL_INC(symbolTableEntry_ptr lvalue);
void HANDLE_TERM_DEC_LVAL(symbolTableEntry_ptr lvalue);
void HANDLE_TERM_LVAL_DEC(symbolTableEntry_ptr lvalue);

//========================

void HANDLE_LVALUE_ID(symbolTableEntry_ptr *lvalue, const char* id);
void HANDLE_LVALUE_LOCAL_ID(symbolTableEntry_ptr *lvalue, const char* id);
void HANDLE_LVALUE_GLOBAL_ID(symbolTableEntry_ptr *lvalue, const char* id);

//========================

void HANDLE_FUNCDECLARE_ID(symbolTableEntry_ptr *lvalue, const char* id);
void HANDLE_FUNCDECLARE_ANON(symbolTableEntry_ptr *lvalue);

#endif

// #include "../include/parser_rules.h"
// #include "../include/symtable.h"
// #include "../include/intermediate.h"
// #include "../include/tools.h"

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// unsigned int anonymousFunctionCounter = 0;

// void RULE_STMT_BREAK(){
//     if(StackIsEmpty(loopStack)){
//         SYNTAX_ERROR("Break statement outside of loop");
//     }
// }

// void RULE_STMT_CONTINUE(){
//     if(StackIsEmpty(loopStack)){
//         SYNTAX_ERROR("Continue statement outside of loop");
//     }
// }

// void RULE_STMT_RETURNSTMT(){
//     if(StackIsEmpty(functionStack)){
//         SYNTAX_ERROR("Return statement outside of function");
//     }
// }

// void RULE_ASSIGNEXPR_LVALUE_ASSIGN_EXPR(SymbolTableEntry_t lvalue){    
//     if(lvalue->type == LIBFUNC){
//         SYNTAX_ERROR("Cannot assign to library function \"%s\"", lvalue->name);
//     }

//     if(lvalue->type == USERFUNC){
//         SYNTAX_ERROR("Cannot assign to user function \"%s\"", lvalue->name);
//     }
// }

// void RULE_PLUSPLUS_LVALUE(SymbolTableEntry_t lvalue){
//     if(lvalue->type == LIBFUNC){
//         SYNTAX_ERROR("Cannot increment library function \"%s\"", lvalue->name);
//     }

//     if(lvalue->type == USERFUNC){
//         SYNTAX_ERROR("Cannot increment user function \"%s\"", lvalue->name);
//     }
// }

// void RULE_LVALUE_PLUSPLUS(SymbolTableEntry_t lvalue){
//     if(lvalue->type == LIBFUNC){
//         SYNTAX_ERROR("Cannot increment library function \"%s\"", lvalue->name);
//     }

//     if(lvalue->type == USERFUNC){
//         SYNTAX_ERROR("Cannot increment user function \"%s\"", lvalue->name);
//     }
// }

// void RULE_MINUSMINUS_LVALUE(SymbolTableEntry_t lvalue){
//     if(lvalue->type == LIBFUNC){
//         SYNTAX_ERROR("Cannot decrement library function \"%s\"", lvalue->name);
//     }

//     if(lvalue->type == USERFUNC){
//         SYNTAX_ERROR("Cannot decrement user function \"%s\"", lvalue->name);
//     }

// }

// void RULE_LVALUE_MINUSMINUS(SymbolTableEntry_t lvalue){
//     if(lvalue->type == LIBFUNC){
//         SYNTAX_ERROR("Cannot decrement library function \"%s\"", lvalue->name);
//     }

//     if(lvalue->type == USERFUNC){
//         SYNTAX_ERROR("Cannot decrement user function \"%s\"", lvalue->name);
//     }
// }

// void RULE_LVALUE_ID(SymbolTableEntry_t* lvalue, const char* id){
//     SymbolTableEntry_t entry = SymtableLookup(id);

//     if(entry == NULL){
//         entry = SymtableInsert(id, scope, yylineno, (scope == 0) ? GLOBALVAR : LOCALVAR);
//     } else {
//         if(!StackIsEmpty(functionStack) && entry->scope != 0 && entry->type != USERFUNC){
//             if(entry->scope <= StackTop(functionStack)){
//                 SYNTAX_ERROR("Variable \"%s\" (at scope %d) mentioned illegaly inside function", id, entry->scope);
//                 return;
//             }
//         }
//     }

//     *lvalue = entry;
// }

// void RULE_LVALUE_LOCAL_ID(SymbolTableEntry_t* lvalue, const char* id){
//     SymbolTableEntry_t entry = SymtableLocalLookup(id);

//     if(entry == NULL){
//         entry = SymtableInsert(id, scope, yylineno, (scope == 0) ? GLOBALVAR : LOCALVAR);
//     } else {
//         if(entry->type == LIBFUNC && scope != 0){
//             SYNTAX_ERROR("Symbol \"%s\" already declared as library function", id);
//             return;
//         }

//     }

//     *lvalue = entry;
// }

// void RULE_GLOBAL_ID(SymbolTableEntry_t* funcdeclare, const char* id){
//     SymbolTableEntry_t entry = SymtableGlobalLookup(id);

//     if(entry == NULL){
//         SYNTAX_ERROR("Symbol \"%s\" cannot be found at scope 0 (global)", id);
//         return;
//     }

//     *funcdeclare = entry;
// }

// void RULE_FUNCDECLARE_ID(SymbolTableEntry_t* funcdeclare, const char* id){
    
//     SymbolTableEntry_t entry;

//     if(id == NULL){
//         char* temp_id;
//         temp_id = malloc(countDigits(anonymousFunctionCounter) + 1 + 2);
//         sprintf(temp_id, "_f%d", anonymousFunctionCounter++);
//         id = temp_id;
//     } 
    
    
//     entry = SymtableLookup(id); 
        
//     if(entry != NULL) {
//         if(entry->type == LIBFUNC){
//             SYNTAX_ERROR("Symbol \"%s\" already declared as library function", id);
//             return;
//         }

//         if(entry->type == USERFUNC && entry->scope == scope){
//             SYNTAX_ERROR("Function \"%s\" already declared", id);
//             return;
//         }

//         if((entry->type == GLOBALVAR || entry->type == LOCALVAR || entry->type == FORMALVAR) && entry->scope == scope){
//             SYNTAX_ERROR("Symbol \"%s\" already declared as variable", id);
//             return;
//         }
//     }

//     entry = SymtableInsert(id, scope, yylineno, USERFUNC);

//     *funcdeclare = entry;
// }

// void RULE_IDLIST_ID(const char* id){
//     SymbolTableEntry_t entry = SymtableLocalLookup(id);

//     if(entry != NULL){
//         if(entry->type == LIBFUNC){
//             SYNTAX_ERROR("Symbol \"%s\" already declared as library function", id);
//             return;
//         }

//         if(entry->type == FORMALVAR){
//             SYNTAX_ERROR("Symbol \"%s\" already declared as formal argument", id);
//             return;
//         }
//     }

//     entry = SymtableInsert(id, scope, yylineno, FORMALVAR);
// }

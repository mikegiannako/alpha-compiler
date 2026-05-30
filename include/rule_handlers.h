#ifndef _RULE_HANDLERS_H_
#define _RULE_HANDLERS_H_

#include "../include/symtable.h"
#include "../include/generic_stack.h"
#include "../include/intermediate.h"

extern unsigned int loopCounter;
extern unsigned int scope;
extern int yylineno;
extern UIntStack_ptr functionScopeStack;
extern UIntStack_ptr loopCounterStack;
extern UIntStack_ptr funcstartJumpStack;
extern UIntStack_ptr tempVarCountStack;
extern UIntStack_ptr scopeOffsetStack;

// ------------------ STMT ------------------
void HANDLE_STMT_GENERIC(Stmt_ptr* stmt);
void HANDLE_STMTLIST(Stmt_ptr* stmt_list, Stmt_ptr parsed_stmts, Stmt_ptr curr_stmt);
void HANDLE_BREAK(Stmt_ptr* stmt);
void HANDLE_CONTINUE(Stmt_ptr* stmt);
void HANDLE_RETURN(Stmt_ptr* stmt, Expr_ptr expr);

// ------------------ EXPR ------------------
void RULE_EXPR_ARITHOP(Expr_ptr* expr, Expr_ptr left, Expr_ptr right, enum yytokentype op);
void RULE_EXPR_RELOP(Expr_ptr* expr, Expr_ptr left, Expr_ptr right, enum yytokentype op);
void RULE_EXPR_AND(Expr_ptr* expr, Expr_ptr left, unsigned int marker, Expr_ptr right);
void RULE_EXPR_OR(Expr_ptr* expr, Expr_ptr left, unsigned int marker, Expr_ptr right);


// ------------------ TERM ------------------
void HANDLE_TERM_UMINUS_EXPR(Expr_ptr* term, Expr_ptr expr);
void HANDLE_TERM_NOT_EXPR(Expr_ptr* term, Expr_ptr expr);
void HANDLE_TERM_INC_LVAL(Expr_ptr* term, Expr_ptr lvalue);
void HANDLE_TERM_LVAL_INC(Expr_ptr* term, Expr_ptr lvalue);
void HANDLE_TERM_DEC_LVAL(Expr_ptr* term, Expr_ptr lvalue);
void HANDLE_TERM_LVAL_DEC(Expr_ptr* term, Expr_ptr lvalue);

// ------------------ ASSIGNEXPR ------------------
void HANDLE_ASSIGNEXPR(Expr_ptr* assignexpr, Expr_ptr lvalue, Expr_ptr expr);

// ------------------ PRIMARY ------------------
void HANDLE_PRIMARY_FUNCDEF(Expr_ptr* primary, SymbolTableEntry_ptr funcdef);

// ------------------ LVALUE ------------------
void HANDLE_LVALUE_ID(Expr_ptr* lvalue, const char* id);
void HANDLE_LVALUE_LOCAL_ID(Expr_ptr* lvalue, const char* id);
void HANDLE_LVALUE_GLOBAL_ID(Expr_ptr* lvalue, const char* id);

// ------------------ MEMBER ------------------
void HANDLE_MEMBER_DOT(Expr_ptr* member, Expr_ptr lvalue, const char* id);
void HANDLE_MEMBER_BRACKET(Expr_ptr* member, Expr_ptr lvalue, Expr_ptr expr);

// ------------------ CALL ------------------
void HANDLE_CALL_LVALUE_CALLSUFFIX(Expr_ptr* call, Expr_ptr lvalue, Call_ptr callsuffix);
void HANDLE_CALL_FUNCDEF_ELIST(Expr_ptr* call, SymbolTableEntry_ptr funcdef, Expr_ptr elist);

// ------------------ OBJECTDEF ------------------
void HANDLE_OBJECTDEF_ELIST(Expr_ptr* objectdef, Expr_ptr elist);
void HANDLE_OBEJCTDEF_INDEXED(Expr_ptr* objectdef, Expr_ptr indexed);

// ------------------ FUNCDEF ------------------
void RULE_FUNCDEF(SymbolTableEntry_ptr* funcdef, SymbolTableEntry_ptr funcdeclare, unsigned int funcparams, unsigned int funcbody);
void HANDLE_FUNCNAME_ID(SymbolTableEntry_ptr* funcname, const char* id);
void HANDLE_FUNCNAME_ANON(SymbolTableEntry_ptr* funcname);
void HANDLE_FUNCDECLARE_FUNCNAME(SymbolTableEntry_ptr* funcdecl, SymbolTableEntry_ptr name_sym);
void HANDLE_FUNCPARAMS(unsigned int* funcparams, unsigned int idlist);
void HANDLE_FUNCBODY(unsigned int* funcbody, Stmt_ptr block);

// ------------------ CONST ------------------

// ------------------ IDLIST ------------------
void HANDLE_IDLIST(unsigned int *idlist, const char* id, unsigned int list_tail);

// ------------------ IF ------------------
void HANDLE_IFSTMT_IF_ELSE_STMT(Stmt_ptr* ifstmt, unsigned int ifprefix, Stmt_ptr stmt1, unsigned int elseprefix, Stmt_ptr stmt2);
void HANDLE_IFSTMT_IFPREFIX_STMT(Stmt_ptr* ifstmt, unsigned int ifprefix, Stmt_ptr stmt);
void HANDLE_IFPREFIX_IF_EXPR(unsigned int* ifprefix, Expr_ptr expr);
void HANDLE_ELSEPREFIX_ELSE(unsigned int* elseprefix);

// ------------------ WHILE ------------------
void HANDLE_WHILESTMT(Stmt_ptr* whilestmt, unsigned int whilestart, unsigned int whileexpr, Stmt_ptr stmt);
void HANDLE_WHILEEXPR(unsigned int* whileexpr, Expr_ptr expr);

// ------------------ FOR ------------------
void RULE_FORSTMT_FORPREFIX_ELIST_STMT(Stmt_ptr* forstmt, ForLoopPrefix_ptr forprefix, unsigned int jump1, Expr_ptr elist,\
                                                                             unsigned int jump2, Stmt_ptr stmt, unsigned int jump3);
void RULE_FORPREFIX_FOR_ELIST_EXPR(ForLoopPrefix_ptr* forprefix, Expr_ptr elist, unsigned int marker, Expr_ptr expr);

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

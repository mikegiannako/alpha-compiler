#ifndef _INTERMEDIATE_INCLUDE_TYPES_
#define _INTERMEDIATE_INCLUDE_TYPES_

#include <stdbool.h>

#include "../include/symtable.h"


typedef enum ExprType {
    VAR_EXPRTYPE,
    TABLE_ITEM_EXPRTYPE,

    PROGRAM_FUNC_EXPRTYPE,
    LIBRARY_FUNC_EXPRTYPE,

    ARITH_EXPR_EXPRTYPE,
    BOOL_EXPR_EXPRTYPE,
    ASSIGN_EXPR_EXPRTYPE,
    NEW_TABLE_EXPRTYPE,

    CONST_NUM_EXPRTYPE,
    CONST_BOOL_EXPRTYPE,
    CONST_STRING_EXPRTYPE,

    NIL_EXPRTYPE
} ExprType_enum;

typedef struct Expr {
    ExprType_enum           type;
    SymbolTableEntry_ptr    sym;
    struct Expr*            index;

    union {
        double              numConst;
        char*               strConst;
        bool                boolConst;
    };

    unsigned int            trueList;
    unsigned int            falseList;

    struct Expr*            next;
}* Expr_ptr;

typedef struct Call {
    Expr_ptr elist;
    const char* method_name;
    bool isMethodCall;
}* Call_ptr;

typedef struct Stmt {
    unsigned int breakList, contList, returnList;
}* Stmt_ptr;

typedef struct ForLoopPrefix {
    unsigned int test;
    unsigned int enter;                 // materialise mode: index of the `if_eq` test
    unsigned int trueList, falseList;   // backpatch mode: condition's pending jump lists
}* ForLoopPrefix_ptr;


#endif 
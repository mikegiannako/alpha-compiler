#ifndef _QUAD_H_
#define _QUAD_H_

#include "intermediate_rule_types.h"
#include <stdbool.h>
#include <stdio.h>

#define NO_LABEL 0

typedef enum QuadOpcode {
    ASSIGN_QUADOP,
    ADD_QUADOP,
    SUB_QUADOP,
    MUL_QUADOP,
    DIV_QUADOP,
    MOD_QUADOP,
    UMINUS_QUADOP,
    IF_EQ_QUADOP,
    IF_NOT_EQ_QUADOP,
    IF_LESS_EQ_QUADOP,
    IF_GREATER_EQ_QUADOP,
    IF_LESS_QUADOP,
    IF_GREATER_QUADOP,
    CALL_QUADOP,
    PARAM_QUADOP,
    RET_QUADOP,
    GET_RET_VAL_QUADOP,
    FUNC_START_QUADOP,
    FUNC_END_QUADOP,
    TABLE_CREATE_QUADOP,
    TABLE_GET_ELEM_QUADOP,
    TABLE_SET_ELEM_QUADOP,
    JUMP_QUADOP,
    NOOP_QUADOP
} QuadOp_enum;

extern const char* quadOpToStr[];

typedef struct Quad {
    QuadOp_enum op;
    Expr_ptr result;
    Expr_ptr arg1;
    Expr_ptr arg2;
    unsigned int label;
    unsigned int line;
}* Quad_ptr;

extern Quad_ptr quadArr;
extern unsigned int totalQuadArrSize;
extern unsigned int nextQuadIndex;

#define QUAD_ARR_EXPAND_SIZE 1024
#define QUAD_ARR_CURR_SIZE (totalQuadArrSize * sizeof(struct Quad))
#define QUAD_ARR_NEW_SIZE (QUAD_ARR_EXPAND_SIZE * sizeof(struct Quad) + QUAD_ARR_CURR_SIZE)

void quad_ExpandArray();
void quad_Emit(
    QuadOp_enum op,
    Expr_ptr result,
    Expr_ptr arg1,
    Expr_ptr arg2,
    unsigned int label
);

unsigned int quad_NextLabel();
bool quadOp_IsBranch(QuadOp_enum op);
void quad_PatchLabel(unsigned int quadIndex, unsigned int quadLabel);

int quadIndexList_New (int quadIndex);
int quadIndexList_Merge (int list1, int list2);
void quadIndexList_Patch (int list, int label);

void quad_PrintSingle(FILE* buffer, Quad_ptr quad, unsigned int quadIndex);
void quad_PrintAll();

#endif
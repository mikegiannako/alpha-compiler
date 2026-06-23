#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../include/quad.h"
#include "../include/utils.h"
#include "error_macros.h"
#include "memory_operations.h"

// Having the quad array be 1-indexed because there could be bugs/issues with unpatched labels which in theory point to quadArr[0]
#define QUAD_ARR_EMPTY_SIZE 1U

Quad_ptr quadArr = NULL;
unsigned int totalQuadArrSize = QUAD_ARR_EMPTY_SIZE;
unsigned int nextQuadIndex = QUAD_ARR_EMPTY_SIZE;

const char* quadOpToStr[] = {
    "assign",
    "add",
    "sub",
    "mul",
    "div",
    "mod",
    "uminus",
    "if_eq",
    "if_noteq",
    "if_lesseq",
    "if_greatereq",
    "if_less",
    "if_greater",
    "call",
    "param",
    "ret",
    "getretval",
    "funcstart",
    "funcend",
    "tablecreate",
    "tablegetelem",
    "tablesetelem",
    "jump",
    "nop"
};

void quad_ExpandArray(){
    assert(nextQuadIndex == totalQuadArrSize);

    quadArr = safeRealloc(quadArr, QUAD_ARR_NEW_SIZE, (nextQuadIndex == QUAD_ARR_EMPTY_SIZE ? "initializing quad array" : "resizing quad array"));

    totalQuadArrSize += QUAD_ARR_EXPAND_SIZE;
}

void quad_Emit(
    QuadOp_enum op,
    Expr_ptr result,
    Expr_ptr arg1,
    Expr_ptr arg2,
    unsigned int label
){
    if(nextQuadIndex == totalQuadArrSize) quad_ExpandArray();

    Quad_ptr temp = &(quadArr[nextQuadIndex++]);
    temp->op = op;
    temp->result = result;
    temp->arg1 = arg1;
    temp->arg2 = arg2;
    temp->label = label;
    temp->line = yylineno;
}

unsigned int quad_NextLabel(){
    return nextQuadIndex;
}

bool quadOp_IsBranch(QuadOp_enum op){
    switch(op){
        case JUMP_QUADOP:
        case IF_EQ_QUADOP:
        case IF_NOT_EQ_QUADOP:
        case IF_LESS_EQ_QUADOP:
        case IF_GREATER_EQ_QUADOP:
        case IF_LESS_QUADOP:
        case IF_GREATER_QUADOP:
            return true;
        default:
            return false;
    }
}

void quad_PatchLabel(unsigned int quadIndex, unsigned int quadLabel){
    assert(quadIndex < nextQuadIndex);

    if(!quadOp_IsBranch(quadArr[quadIndex].op)){
        WARNING_MSG("COMPILER", "Patching label on non-branch quad #%d (opcode: %s)", quadIndex, quadOpToStr[quadArr[quadIndex].op]);
    }

    quadArr[quadIndex].label = quadLabel;
}

int quadIndexList_New(int quadIndex){
    quadArr[quadIndex].label = 0;
    return quadIndex;
}

int quadIndexList_Merge(int list1, int list2){
    if(!list1) return list2;
    if(!list2) return list1;

    int i = list1;

    while(quadArr[i].label){
        i = quadArr[i].label;
    }

    quadArr[i].label = list2;

    return list1;
}

void quadIndexList_Patch(int list, int label){
    unsigned int next;
    while(list){
        next = quadArr[list].label;
        if(!quadOp_IsBranch(quadArr[list].op)){
            WARNING_MSG("COMPILER", "Patching label on non-branch quad #%d (opcode: %s) via index list", list, quadOpToStr[quadArr[list].op]);
        }
        quad_PatchLabel(list, label);
        list = next;
    }
}

// -------------------------- Printing ----------------------------------


char* exprToStr(Expr_ptr expr){
    char* result;

    if(!expr){
        return safeStrDup("\0", "duplicating \\0 to not have different \"free\" treatment for empty strings");
    }

    switch(expr->type){
        case VAR_EXPRTYPE:
        case TABLE_ITEM_EXPRTYPE:
        case PROGRAM_FUNC_EXPRTYPE:
        case LIBRARY_FUNC_EXPRTYPE:
        case ARITH_EXPR_EXPRTYPE:
        case BOOL_EXPR_EXPRTYPE:
        case ASSIGN_EXPR_EXPRTYPE:
        case NEW_TABLE_EXPRTYPE:
            if(!expr->sym){
                return safeStrDup("\0", "duplicating \\0 to not have different \"free\" treatment for empty strings");
            }
            result = safeStrDup(expr->sym->name, "duplicating table/object name");
            break;
        case CONST_NUM_EXPRTYPE:
            result = safeCalloc(1, (snprintf(NULL, 0, "%f", expr->numConst) + 1), "allocating space for a num const to string conversion");
            sprintf(result, "%0.*f" , ((expr->numConst == (int) expr->numConst) ? 0 : 4), expr->numConst);
            break;
        case CONST_BOOL_EXPRTYPE:
            result = safeStrDup((expr->boolConst ? "true" : "false"), "duplicating true/false strings not have different \"free\" treatment from other string conversions");
            break;
        case CONST_STRING_EXPRTYPE:
            result = safeCalloc(1, (strlen(expr->strConst) + 1 + 2), "allocating space for a string const that includes quotes");
            sprintf(result, "\"%s\"", expr->strConst); 
            break;
        case NIL_EXPRTYPE:
            result = safeStrDup("nil", "duplicating \"nil\" string to not have different \"free\" treatment from other string conversions");
            break;
        default:
            result = safeStrDup("nil", "duplicating \"unknown\" string to not have different \"free\" treatment from other string conversions");
            break;
    }

    return result;
}


void quad_PrintSingle(FILE* buffer, Quad_ptr quad, unsigned int quadIndex){
    if(quad == NULL){
        fprintf(buffer, "NULL\n");
    }

    fprintf(buffer, "%d: ", quadIndex);
    FillSpace(buffer, "", 6 - countDigits(quadIndex), " ");
    fprintf(buffer, "%-15s ", quadOpToStr[quad->op]);
    

    char* result = exprToStr(quad->result);
    fprintf(buffer, "%-15s ", result);
    safeFree(&result, "freeing memory for quad result string");

    char* arg1 = exprToStr(quad->arg1);
    fprintf(buffer, "%-20s ", arg1);
    safeFree(&arg1, "freeing memory for quad arg1 string");


    char* arg2 = exprToStr(quad->arg2);
    fprintf(buffer, "%-20s ", arg2);
    safeFree(&arg2, "freeing memory for quad arg2 string");


    if(quad->label) fprintf(buffer, "%-10d ", quad->label);
    else FillSpace(buffer, "", 11, " ");
    fprintf(buffer, "%-5d\n", quad->line);

    if(quad->label == 0 && quadOp_IsBranch(quad->op)){
        WARNING_MSG("COMPILER", "Quad #%d had an unpatched label", quadIndex);
    }
}

void quad_PrintAll(){
    #ifdef DEBUG
        printf("Total quads: %d\n", nextQuadIndex - 1);
    #endif
    printf("quad#   opcode");
    FillSpace(stdout, "opcode", 16, " ");
    printf("result");
    FillSpace(stdout, "result", 16, " ");
    printf("arg1");
    FillSpace(stdout, "arg1", 21, " ");
    printf("arg2");
    FillSpace(stdout, "arg2", 21, " ");
    printf("label");
    FillSpace(stdout, "label", 11, " ");
    printf("line\n");
    FillSpace(stdout, "", 16 + 16 + 21 + 21 + 11 + 6 + 7, "-");
    printf("\n");

    FILE* quadsFile = fopen("quads.txt", "w");
    for(unsigned int i = 1; i < nextQuadIndex; i++){
        quad_PrintSingle(stdout, quadArr + i, i);
        quad_PrintSingle(quadsFile, quadArr + i, i);
    }
    fclose(quadsFile);
}
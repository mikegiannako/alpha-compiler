#ifndef _UINT_STACK_H_
#define _UINT_STACK_H_

#include <stdio.h>

typedef struct uiStack {
    unsigned int data;
    struct uiStack *next;
}* uiStack_ptr;

void uiStack_Push(uiStack_ptr *stack, unsigned int data);
unsigned int uiStack_Pop(uiStack_ptr *stack);
unsigned int uiStack_Top(uiStack_ptr stack);
int uiStack_IsEmpty(uiStack_ptr stack);
void uiStack_Clear(uiStack_ptr stack);

#endif
#include <stdio.h>
#include <stdlib.h>

#include "../include/uint_stack.h"
#include "../include/memory_operations.h"
#include "../include/error_macros.h"

void uiStack_Push(uiStack_ptr *stack, unsigned int data) {
    uiStack_ptr newNode = safeCalloc(1, sizeof(struct uiStack), "allocating memory for new stack node");
    newNode->data = data;
    newNode->next = *stack;
    *stack = newNode;
}

unsigned int uiStack_Pop(uiStack_ptr *stack) {
    if (uiStack_IsEmpty(*stack)) {
        ERROR_MSG(__FILE__, __LINE__, "STACK", "Stack underflow while %s", "popping from stack");
    }

    uiStack_ptr temp = *stack;
    unsigned int poppedValue = temp->data;
    *stack = (*stack)->next;

    safeFree(&temp, "freeing popped stack node");

    return poppedValue;
}

unsigned int uiStack_Top(uiStack_ptr stack) {
    if (uiStack_IsEmpty(stack)) {
        ERROR_MSG(__FILE__, __LINE__, "STACK", "Stack is empty while %s", "trying to access top element");
    }
    return stack->data;
}

int uiStack_IsEmpty(uiStack_ptr stack) {
    return stack == NULL;
}

void uiStack_Clear(uiStack_ptr stack){
    while (!uiStack_IsEmpty(stack)) {
        uiStack_Pop(&stack);
    }
}
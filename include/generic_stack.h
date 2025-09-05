#ifndef _GENERIC_STACK_H_
#define _GENERIC_STACK_H_

#include <stdio.h>
#include <stdlib.h>

#include "memory_operations.h"
#include "error_macros.h"

// Macro to declare stack types and functions
#define DECLARE_STACK(type, prefix) \
    typedef struct prefix##_node { \
        type data; \
        struct prefix##_node *next; \
    } *prefix##_ptr; \
    \
    void prefix##_Push(prefix##_ptr *stack, type data); \
    type prefix##_Pop(prefix##_ptr *stack); \
    type prefix##_Top(prefix##_ptr stack); \
    int prefix##_IsEmpty(prefix##_ptr stack); \
    void prefix##_Clear(prefix##_ptr *stack);

// Macro to implement stack functions (using your helper macros/functions)
#define IMPLEMENT_STACK(type, prefix) \
    void prefix##_Push(prefix##_ptr *stack, type data) { \
        prefix##_ptr newNode = safeCalloc(1, sizeof(struct prefix##_node), "allocating memory for new stack node"); \
        newNode->data = data; \
        newNode->next = *stack; \
        *stack = newNode; \
    } \
    \
    type prefix##_Pop(prefix##_ptr *stack) { \
        if (prefix##_IsEmpty(*stack)) { \
            ERROR_MSG(__FILE__, __LINE__, "STACK", "Stack underflow while %s", "popping from stack"); \
        } \
        \
        prefix##_ptr temp = *stack; \
        type poppedValue = temp->data; \
        *stack = (*stack)->next; \
        \
        safeFree(&temp, "freeing popped stack node"); \
        \
        return poppedValue; \
    } \
    \
    type prefix##_Top(prefix##_ptr stack) { \
        if (prefix##_IsEmpty(stack)) { \
            ERROR_MSG(__FILE__, __LINE__, "STACK", "Stack is empty while %s", "trying to access top element"); \
        } \
        return stack->data; \
    } \
    \
    int prefix##_IsEmpty(prefix##_ptr stack) { \
        return stack == NULL; \
    } \
    \
    void prefix##_Clear(prefix##_ptr *stack) { \
        while (!prefix##_IsEmpty(*stack)) { \
            prefix##_Pop(stack); \
        } \
    }

DECLARE_STACK(unsigned int, uintStack)
DECLARE_STACK(int*, intPtrStack)

#endif
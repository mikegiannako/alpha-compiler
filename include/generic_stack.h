#ifndef _GENERIC_STACK_H_
#define _GENERIC_STACK_H_

#include <stdio.h>
#include <stdlib.h>

#include "memory_operations.h"
#include "error_macros.h"

// Macro to declare stack types and functions
#define DECLARE_STACK(type, name, prefix) \
    typedef struct name##_node { \
        type data; \
        struct name##_node *next; \
    } *name##_ptr; \
    \
    void prefix##_Push(name##_ptr *stack, type data); \
    type prefix##_Pop(name##_ptr *stack); \
    type prefix##_Top(name##_ptr stack); \
    int prefix##_IsEmpty(name##_ptr stack); \
    void prefix##_Clear(name##_ptr *stack);

// Macro to implement stack functions (using your helper macros/functions)
#define IMPLEMENT_STACK(type, name, prefix) \
    void prefix##_Push(name##_ptr *stack, type data) { \
        name##_ptr newNode = safeCalloc(1, sizeof(struct name##_node), "allocating memory for new stack node"); \
        newNode->data = data; \
        newNode->next = *stack; \
        *stack = newNode; \
    } \
    \
    type prefix##_Pop(name##_ptr *stack) { \
        if (prefix##_IsEmpty(*stack)) { \
            ERROR_MSG("STACK", "Stack underflow while %s", "popping from stack"); \
        } \
        \
        name##_ptr temp = *stack; \
        type poppedValue = temp->data; \
        *stack = (*stack)->next; \
        \
        safeFree(&temp, "freeing popped stack node"); \
        \
        return poppedValue; \
    } \
    \
    type prefix##_Top(name##_ptr stack) { \
        if (prefix##_IsEmpty(stack)) { \
            ERROR_MSG("STACK", "Stack is empty while %s", "trying to access top element"); \
        } \
        return stack->data; \
    } \
    \
    int prefix##_IsEmpty(name##_ptr stack) { \
        return stack == NULL; \
    } \
    \
    void prefix##_Clear(name##_ptr *stack) { \
        while (!prefix##_IsEmpty(*stack)) { \
            prefix##_Pop(stack); \
        } \
    }

DECLARE_STACK(unsigned int, UIntStack, uintStack)
DECLARE_STACK(int*, IntPtrStack, intPtrStack)

#endif
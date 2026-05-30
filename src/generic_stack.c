#include "../include/generic_stack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/memory_operations.h"
#include "../include/error_macros.h"

IMPLEMENT_STACK(unsigned int, UIntStack, uintStack)
IMPLEMENT_STACK(int*, IntPtrStack, intPtrStack)
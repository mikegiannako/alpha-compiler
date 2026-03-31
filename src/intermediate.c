#include <stdbool.h>

#include "../include/intermediate.h"
#include "../include/memory_operations.h"
#include "../include/error_macros.h"

Call_ptr createCallValue(bool isMethodCall, const char* method_name){
    Call_ptr temp = safeCalloc(1, sizeof(struct call), "creating Call object");

    temp->method_name = method_name ? method_name : NULL;
    temp->isMethodCall = isMethodCall;

    return temp;
}

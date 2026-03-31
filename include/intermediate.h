#ifndef _INTERMEDIATE_
#define _INTERMEDIATE_

#include <stdbool.h>

typedef struct call {
    // TODO: add more fields when needed later
    
    const char* method_name;
    bool isMethodCall;
}* Call_ptr;

Call_ptr createCallValue(bool isMethodCall, const char* method_name);

#endif 
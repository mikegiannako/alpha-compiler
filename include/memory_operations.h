#ifndef _MEMORY_OPERATIONS_H_
#define _MEMORY_OPERATIONS_H_

#include <stdio.h>

// Internal functions - don't call these directly
void* _safeCalloc(size_t num, size_t size, const char* desc, const char* file, int line);
void* _safeRealloc(void* ptr, size_t newSize, const char* desc, const char* file, int line);
void _safeFree(void** ptr, const char* desc, const char* file, int line);

// Macros that automatically include file and line information
#define safeCalloc(num, size, desc) \
    _safeCalloc(num, size, desc, __FILE__, __LINE__)

#define safeRealloc(ptr, newSize, desc) \
    _safeRealloc(ptr, newSize, desc, __FILE__, __LINE__)

#define safeFree(ptr, desc) \
    _safeFree((void**)(ptr), desc, __FILE__, __LINE__)

#endif // _MEMORY_OPERATIONS_H_
#include <stdio.h>
#include <stdlib.h>

#include "../include/memory_operations.h"
#include "../include/error_macros.h"

void* _safeCalloc(size_t num, size_t size, const char* desc, const char* file, int line) {
    void* ptr = calloc(num, size);

    if(!ptr) ERROR_MSG(file, line, "MEMORY", "Calloc failed while %s", desc);

    return ptr;
}

void* _safeRealloc(void* ptr, size_t newSize, const char* desc, const char* file, int line) {
    // Handle realloc(NULL, size) case
    if (!ptr) {
        return safeCalloc(1, newSize, desc);
    }
    
    // Handle realloc(ptr, 0) case - equivalent to free
    if (newSize == 0) {
        safeFree(ptr, desc);
        return NULL;
    }
    
    void* newPtr = realloc(ptr, newSize);

    if (!newPtr) {
        ERROR_MSG(file, line, "MEMORY", "Realloc failed while %s (old ptr: %p, new size: %zu)", desc, ptr, newSize);
    }
        
    return newPtr;
}

void _safeFree(void** ptr, const char* desc, const char* file, int line) {
    if (!ptr) {
        WARNING_MSG(file, line, "LOGIC", "Null was provided as a pointer-to-pointer while %s", desc);
        return;
    }
    
    if (!*ptr) {
        // Freeing NULL is valid in C, so just return silently
        WARNING_MSG(file, line, "LOGIC", "Attempted to free a NULL pointer while %s", desc);
        return;
    }
    
    free(*ptr);
    *ptr = NULL;
}

char* _safeStrDup(const char* src, const char* desc, const char* file, int line){
    if (!src) {
        ERROR_MSG(file, line, "LOGIC", "Null source string provided while %s", desc);
        return NULL;
    }

    char* copy = strdup(src);
    if (!copy) {
        ERROR_MSG(file, line, "MEMORY", "String duplication failed while %s", desc);
    }

    return copy;
}
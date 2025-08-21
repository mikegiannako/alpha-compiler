#include <stdio.h>
#include "../include/lexer.h"
#include "../include/memory_operations.h"

#define INITIAL_BUFFER_CAPACITY 32

strBuffer_ptr strBuffer_Init(){
    strBuffer_ptr buf = safeCalloc(1, sizeof(struct strBuffer), "allocating memory for strBuffer");

    buf->data = safeCalloc(INITIAL_BUFFER_CAPACITY, sizeof(char), "initializing string buffer data");
    buf->length = 0;
    buf->capacity = INITIAL_BUFFER_CAPACITY;

    return buf;
}

void strBuffer_AppendChar(strBuffer_ptr buf, char c){
    if (buf->length >= buf->capacity) {
        buf->capacity = buf->capacity * 2;
        buf->data = safeRealloc(buf->data, buf->capacity, "resizing string buffer data");
    }

    buf->data[buf->length++] = c;    
}

char* strBuffer_FinalizeString(strBuffer_ptr buf){
    if (buf->length == 0) {
        safeFree(&buf->data, "freeing empty (not NULL) string buffer data");
    }
    
    // Shrink to exact size (including null terminator)
    buf->data = safeRealloc(buf->data, (buf->length + 1), "resizing string buffer data to perfectly fit the string (including null terminator)");
    buf->data[buf->length] = '\0';
    
    return buf->data;
}

void strBuffer_Free(strBuffer_ptr buf){
    if (!buf) return; // Avoid freeing NULL

    safeFree(&buf->data, "freeing string buffer data");
    safeFree(&buf, "freeing string buffer struct");
}
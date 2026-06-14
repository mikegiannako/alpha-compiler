#include <stdio.h>
#include "../include/lexer.h"
#include "../include/memory_operations.h"

#define INITIAL_BUFFER_CAPACITY 32

StrBuffer_ptr strBuffer_Init(){
    StrBuffer_ptr buf = safeCalloc(1, sizeof(struct StrBuffer), "allocating memory for strBuffer");

    buf->data = safeCalloc(INITIAL_BUFFER_CAPACITY, sizeof(char), "initializing string buffer data");
    buf->length = 0;
    buf->capacity = INITIAL_BUFFER_CAPACITY;

    return buf;
}

void strBuffer_AppendChar(StrBuffer_ptr buf, char c){
    if (buf->length >= buf->capacity) {
        buf->capacity = buf->capacity * 2;
        buf->data = safeRealloc(buf->data, buf->capacity, "resizing string buffer data");
    }

    buf->data[buf->length++] = c;    
}

char* strBuffer_FinalizeString(StrBuffer_ptr buf){
    if (buf->length == 0) {
        safeFree(&buf->data, "freeing empty (not NULL) string buffer data");
    }
    
    // Shrink to exact size (including null terminator)
    buf->data = safeRealloc(buf->data, (buf->length + 1), "resizing string buffer data to perfectly fit the string (including null terminator)");
    buf->data[buf->length] = '\0';
    
    return buf->data;
}

void strBuffer_Free(StrBuffer_ptr buf){
    if (!buf) return; // Avoid freeing NULL

    safeFree(&buf->data, "freeing string buffer data");
    safeFree(&buf, "freeing string buffer struct");
}

// ----------------- Tracking of lexer-owned strings -----------------

typedef struct TrackedString {
    char* str;
    struct TrackedString* next;
}* TrackedString_ptr;

static TrackedString_ptr trackedStrings = NULL;

char* lexer_TrackString(char* str){
    TrackedString_ptr node = safeCalloc(1, sizeof(struct TrackedString), "allocating tracked lexer string node");
    node->str = str;
    node->next = trackedStrings;
    trackedStrings = node;
    return str;
}

void lexer_FreeTrackedStrings(void){
    TrackedString_ptr node = trackedStrings;
    while(node){
        TrackedString_ptr next = node->next;
        safeFree(&node->str, "freeing tracked lexer string");
        safeFree(&node, "freeing tracked lexer string node");
        node = next;
    }
    trackedStrings = NULL;
}
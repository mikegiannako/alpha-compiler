#ifndef _LEXER_H_
#define _LEXER_H_

typedef struct StrBuffer {
    char *data;
    size_t length;
    size_t capacity;
}* StrBuffer_ptr;

StrBuffer_ptr strBuffer_Init();
void strBuffer_AppendChar(StrBuffer_ptr buf, char c);
char* strBuffer_FinalizeString(StrBuffer_ptr buf);
void strBuffer_Free(StrBuffer_ptr buf);

#endif // _LEXER_H_
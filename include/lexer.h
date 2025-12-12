#ifndef _LEXER_H_
#define _LEXER_H_

typedef struct strBuffer {
    char *data;
    size_t length;
    size_t capacity;
}* strBuffer_ptr;

strBuffer_ptr strBuffer_Init();
void strBuffer_AppendChar(strBuffer_ptr buf, char c);
char* strBuffer_FinalizeString(strBuffer_ptr buf);
void strBuffer_Free(strBuffer_ptr buf);

#endif // _LEXER_H_
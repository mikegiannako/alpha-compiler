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

// Tracking of strings strdup'd by the lexer into yylval. Consumers (symbol table,
// const-string exprs) always copy these, so the originals are never freed
// otherwise. Every tracked string is collected in a global list and released in
// one pass via lexer_FreeTrackedStrings() at the end of compilation.
char* lexer_TrackString(char* str);
void lexer_FreeTrackedStrings(void);

#endif // _LEXER_H_
%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #include "../include/lexer.h"
    #include "../include/utils.h"
    #include "../include/uint_stack.h"
    #include "../include/error_macros.h"
    #include "../build/parser.h"

    strBuffer_ptr stringBuffer;
    unsigned int stringStartLine;
    uiStack_ptr lineStack = NULL;

    char getEscapeChar(char c);
%}

%option yylineno
%option	noyywrap

%x STRING_STATE
%x MULTI_COMMENT_STATE

INTEGER  [0-9]+
REAL     [0-9]*\.[0-9]+([eE][-+]?[0-9]+)?
IDENT 	 [a-zA-Z][0-9a-zA-Z_]*

SINGLE_COMMENT 	"//".*
MULTI_COMMENT   "/\*"
END_MULTI_COMMENT "\*/"

IGNORE         [ \t\n\r]

%%

"="              { return ASSIGN; }
"+"              { return ADD; }
"-"              { return MINUS; }
"*"              { return MUL; }
"/"              { return DIV; }
"%"              { return MOD; }
"=="             { return EQUAL; }
"!="             { return NOT_EQUAL; }
"++"             { return INCREMENT; }
"--"             { return DECREMENT; }
">"              { return GREATER; }
"<"              { return LESS; }
">="             { return GREATER_EQUAL; }
"<="             { return LESS_EQUAL; }
"{"              { return LEFT_BRACKET; }
"}"              { return RIGHT_BRACKET; }
"["              { return LEFT_SQUARE; }
"]"              { return RIGHT_SQUARE; }
"("              { return LEFT_PARENTHESIS; }
")"              { return RIGHT_PARENTHESIS; }
";"              { return SEMICOLON; }
","              { return COMMA; }
":"              { return COLON; }
"::"             { return DOUBLE_COLON; }
"."              { return DOT; }
".."             { return DOUBLE_DOT; }
"if"             { return IF; }
"else"           { return ELSE; }
"while"          { return WHILE; }
"for"            { return FOR; }
"function"       { return FUNCTION; }
"return"         { return RETURN; }
"break"          { return BREAK; }
"continue"       { return CONTINUE; }
"and"            { return AND; }
"not"            { return NOT; }
"or"             { return OR; }
"local"          { return LOCAL; }
"true"           { return TRUE; }
"false"          { return FALSE; }
"nil"            { return NIL; }

{REAL} {
    yylval.numValue = atof(yytext);
    return REAL;
}

{INTEGER} {
    yylval.numValue = atoi(yytext);
    return INTEGER;
}

{IDENT} {
    yylval.stringValue = strdup(yytext);
    return ID;
}

{SINGLE_COMMENT} {}

\" {
    stringBuffer = strBuffer_Init(); // Initialize the string buffer
    stringStartLine = yylineno;      // Store the line number where the string starts

    BEGIN(STRING_STATE);
}

<STRING_STATE>\" {

    char* finalizedString = strBuffer_FinalizeString(stringBuffer);
    yylval.stringValue = strdup(finalizedString);
    strBuffer_Free(stringBuffer); // Free the string buffer

    BEGIN(INITIAL);
    return STRING;
}

<STRING_STATE><<EOF>> {
    ERROR_MSG(__FILE__, __LINE__, "LEXER", "Unterminated string literal starting at line %d", stringStartLine);
    return -1;
}

<STRING_STATE>. {
    char temp = yytext[0];

    if(temp == '\\'){
        char next = input();

        // check if we hit eof
        if(next == EOF){
            ERROR_MSG(__FILE__, __LINE__, "LEXER", "Unterminated escape sequence in string starting at line %d", stringStartLine);
        }

        temp = getEscapeChar(next);
    }

    strBuffer_AppendChar(stringBuffer, temp);
}

{MULTI_COMMENT} {
    uiStack_Push(&lineStack, yylineno);
    BEGIN(MULTI_COMMENT_STATE);
}

<MULTI_COMMENT_STATE>{MULTI_COMMENT} {
    uiStack_Push(&lineStack, yylineno);
}

<MULTI_COMMENT_STATE>{END_MULTI_COMMENT} {
    unsigned int startLine = uiStack_Pop(&lineStack);
    unsigned int startDigits = countDigits(startLine);
    unsigned int endDigits = countDigits(yylineno);
    unsigned int bufSize = startDigits + endDigits + 4; // 4 for " - "
    char buf[bufSize];

    sprintf(buf, "%d - %d", startLine, yylineno);
    if(uiStack_IsEmpty(lineStack)){
        // insertTokenInList(&token_list, MULTI_COMMENT, buf, "BLOCKED_COMMENT", yylineno);
        BEGIN(INITIAL);
    } else {
        // insertTokenInList(&token_list, NESTED_COMMENT, buf, "NESTED_COMMENT", yylineno);
    }
}

<MULTI_COMMENT_STATE>\n {}

<MULTI_COMMENT_STATE>. {}

<MULTI_COMMENT_STATE><<EOF>> {
    uiStack_Clear(lineStack);
    ERROR_MSG(__FILE__, __LINE__, "LEXER", "Unterminated multi-line comment starting at line %d", uiStack_Top(lineStack));
}

{IGNORE} {}

. {
    ERROR_MSG(__FILE__, __LINE__, "LEXER", "Unexpected character '%c' at line %d", yytext[0], yylineno);
}

%%

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s at line %d\n", s, yylineno);
    exit(EXIT_FAILURE);
}

char getEscapeChar(char c){
    switch(c){
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case 'b': return '\b';
        case 'a': return '\a';
        case '\\': return '\\';
        case '\'': return '\'';
        case '\"': return '\"';
        default:{
            unput(c);
            return '\\';
        }
    }
}
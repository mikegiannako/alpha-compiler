%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #include "../include/lexer.h"
    #include "../include/utils.h"
    #include "../include/generic_stack.h"
    #include "../include/error_macros.h"
    #include "../include/memory_operations.h"
    #include "../build/parser.h"

    strBuffer_ptr stringBuffer;
    unsigned int stringStartLine;
    uintStack_ptr lineStack = NULL;

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

"="              { return ASSIGN_TOK; }
"+"              { return ADD_TOK; }
"-"              { return MINUS_TOK; }
"*"              { return MUL_TOK; }
"/"              { return DIV_TOK; }
"%"              { return MOD_TOK; }
"=="             { return EQUAL_TOK; }
"!="             { return NOT_EQUAL_TOK; }
"++"             { return INCREMENT_TOK; }
"--"             { return DECREMENT_TOK; }
">"              { return GREATER_TOK; }
"<"              { return LESS_TOK; }
">="             { return GREATER_EQUAL_TOK; }
"<="             { return LESS_EQUAL_TOK; }
"{"              { return LEFT_BRACKET_TOK; }
"}"              { return RIGHT_BRACKET_TOK; }
"["              { return LEFT_SQUARE_TOK; }
"]"              { return RIGHT_SQUARE_TOK; }
"("              { return LEFT_PARENTHESIS_TOK; }
")"              { return RIGHT_PARENTHESIS_TOK; }
";"              { return SEMICOLON_TOK; }
","              { return COMMA_TOK; }
":"              { return COLON_TOK; }
"::"             { return DOUBLE_COLON_TOK; }
"."              { return DOT_TOK; }
".."             { return DOUBLE_DOT_TOK; }
"if"             { return IF_TOK; }
"else"           { return ELSE_TOK; }
"while"          { return WHILE_TOK; }
"for"            { return FOR_TOK; }
"function"       { return FUNCTION_TOK; }
"return"         { return RETURN_TOK; }
"break"          { return BREAK_TOK; }
"continue"       { return CONTINUE_TOK; }
"and"            { return AND_TOK; }
"not"            { return NOT_TOK; }
"or"             { return OR_TOK; }
"local"          { return LOCAL_TOK; }
"true"           { return TRUE_TOK; }
"false"          { return FALSE_TOK; }
"nil"            { return NIL_TOK; }

{REAL} {
    yylval.numValue = atof(yytext);
    return REAL_TOK;
}

{INTEGER} {
    yylval.numValue = atoi(yytext);
    return INTEGER_TOK;
}

{IDENT} {
    // TODO: perhaps implement global list of string pointer that need to be freed at the end of the program
    yylval.stringValue = safeStrDup(yytext, "copying ID name to yylval");
    return ID_TOK;
}

{SINGLE_COMMENT} {}

\" {
    stringBuffer = strBuffer_Init(); // Initialize the string buffer
    stringStartLine = yylineno;      // Store the line number where the string starts

    BEGIN(STRING_STATE);
}

<STRING_STATE>\" {

    char* finalizedString = strBuffer_FinalizeString(stringBuffer);
    // TODO: perhaps implement global list of string pointer that need to be freed at the end of the program
    yylval.stringValue = safeStrDup(finalizedString, "copying finalized string to yylval");
    strBuffer_Free(stringBuffer); // Free the string buffer

    BEGIN(INITIAL);
    return STRING_TOK;
}

<STRING_STATE><<EOF>> {
    ERROR_MSG("LEXER", "Unterminated string literal starting at line %d", stringStartLine);
    return -1;
}

<STRING_STATE>. {
    char temp = yytext[0];

    if(temp == '\\'){
        char next = input();

        // check if we hit eof
        if(next == EOF){
            ERROR_MSG("LEXER", "Unterminated escape sequence in string starting at line %d", stringStartLine);
        }

        temp = getEscapeChar(next);
    }

    strBuffer_AppendChar(stringBuffer, temp);
}

{MULTI_COMMENT} {
    uintStack_Push(&lineStack, yylineno);
    BEGIN(MULTI_COMMENT_STATE);
}

<MULTI_COMMENT_STATE>{MULTI_COMMENT} {
    uintStack_Push(&lineStack, yylineno);
}

<MULTI_COMMENT_STATE>{END_MULTI_COMMENT} {
    unsigned int startLine = uintStack_Pop(&lineStack);
    unsigned int startDigits = countDigits(startLine);
    unsigned int endDigits = countDigits(yylineno);
    unsigned int bufSize = startDigits + endDigits + 4; // 4 for " - "
    char buf[bufSize];

    sprintf(buf, "%d - %d", startLine, yylineno);
    if(uintStack_IsEmpty(lineStack)){
        // insertTokenInList(&token_list, MULTI_COMMENT, buf, "BLOCKED_COMMENT", yylineno);
        BEGIN(INITIAL);
    } else {
        // insertTokenInList(&token_list, NESTED_COMMENT, buf, "NESTED_COMMENT", yylineno);
    }
}

<MULTI_COMMENT_STATE>\n {}

<MULTI_COMMENT_STATE>. {}

<MULTI_COMMENT_STATE><<EOF>> {
    uintStack_Clear(&lineStack);
    ERROR_MSG("LEXER", "Unterminated multi-line comment starting at line %d", uintStack_Top(lineStack));
}

{IGNORE} {}

. {
    ERROR_MSG("LEXER", "Unexpected character '%c' at line %d", yytext[0], yylineno);
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
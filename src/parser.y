%{
    #include <stdio.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <string.h>
    #include <assert.h>
    #include "../include/lexer.h"
    #include "../include/utils.h"
    #include "../include/generic_stack.h"
	#include "../include/symtable.h"
    #include "../include/rule_handlers.h"

    #ifdef DEBUG
        #define RULE_PRINT(rule) {fprintf(stderr, "LINE: %d ", yylineno); fprintf(stderr, rule);}
    #else
        #define RULE_PRINT(rule) {}
    #endif

	// #define YY_DECL int alpha_yylex (void* ylval)
    
    int yyerror(const char* s);
    int yylex(void);

    extern char* yytext;
    extern FILE* yyin;
    extern FILE* yyout;

    uintStack_ptr functionScopeStack = NULL;
    uintStack_ptr loopCounterStack = NULL;
    uintStack_ptr funcstartJumpStack = NULL;
    uintStack_ptr tempVarCountStack = NULL;
    unsigned int loopCounter = 0;
    unsigned int scope = 0;
%}

%start program

%union{
	const char* stringValue;
	float numValue;
    unsigned int uintValue;
    struct symbolTableEntry* symbolValue;
}

%token <stringValue> STRING_TOK ID_TOK
%token <numValue> REAL_TOK INTEGER_TOK

%token SINGLE_COMMENT_TOK ASSIGN_TOK ADD_TOK MINUS_TOK MUL_TOK DIV_TOK MOD_TOK EQUAL_TOK NOT_EQUAL_TOK INCREMENT_TOK DECREMENT_TOK GREATER_TOK GREATER_EQUAL_TOK
%token LESS_TOK LESS_EQUAL_TOK LEFT_BRACKET_TOK RIGHT_BRACKET_TOK LEFT_SQUARE_TOK RIGHT_SQUARE_TOK LEFT_PARENTHESIS_TOK RIGHT_PARENTHESIS_TOK
%token SEMICOLON_TOK COMMA_TOK COLON_TOK DOUBLE_COLON_TOK DOT_TOK DOUBLE_DOT_TOK ELSE_TOK
%token IF_TOK WHILE_TOK FOR_TOK FUNCTION_TOK RETURN_TOK BREAK_TOK CONTINUE_TOK AND_TOK NOT_TOK OR_TOK LOCAL_TOK TRUE_TOK FALSE_TOK NIL_TOK
%token UMINUS_TOK

%right ASSIGN_TOK 
%right OR_TOK
%left AND_TOK
%nonassoc EQUAL_TOK NOT_EQUAL_TOK
%nonassoc GREATER_TOK GREATER_EQUAL_TOK LESS_TOK LESS_EQUAL_TOK
%left ADD_TOK MINUS_TOK
%left MUL_TOK DIV_TOK MOD_TOK
%right UMINUS_TOK NOT_TOK INCREMENT_TOK DECREMENT_TOK
%left DOT_TOK DOUBLE_DOT_TOK
%left LEFT_BRACKET_TOK RIGHT_BRACKET_TOK
%left LEFT_PARENTHESIS_TOK RIGHT_PARENTHESIS_TOK

%type<symbolValue> lvalue

%%


program:        stmt_list               { RULE_PRINT("program <- stmt_list\n" );}
		        ;

stmt_list:      stmt_list stmt          { RULE_PRINT("statement list <- statement statement list\n");}
                | stmt 		            { RULE_PRINT("statement list <- statement\n");}
                ;   

break:          BREAK_TOK SEMICOLON_TOK         { HANDLE_BREAK(); RULE_PRINT("break <- BREAK ;\n");}
                ;   

continue:       CONTINUE_TOK SEMICOLON_TOK      { HANDLE_CONTINUE(); RULE_PRINT("continue <- CONTINUE ;\n");}
                ;   

returnstmt:     RETURN_TOK expr SEMICOLON_TOK   { HANDLE_RETURN(); RULE_PRINT("returnstmt <- RETURN expression ;\n");}
                | RETURN_TOK SEMICOLON_TOK      { HANDLE_RETURN(); RULE_PRINT("returnstmt <- RETURN ;\n");}
                ;   

stmt:           expr SEMICOLON_TOK      { RULE_PRINT("statement <- expression ;\n");}
                | ifstmt 		        { RULE_PRINT("statement <- if\n");}
                | whilestmt 		    { RULE_PRINT("statement <- while\n");}
                | forstmt 		        { RULE_PRINT("statement <- for\n");}
                | returnstmt            { RULE_PRINT("statement <- return\n");}
                | break                 { RULE_PRINT("statement <- break ;\n");}
                | continue              { RULE_PRINT("statement <- continue ;\n");}
                | block 		        { RULE_PRINT("statement <- block\n");}
                | funcdef 		        { RULE_PRINT("statement <- function definition\n");}
                | SEMICOLON_TOK 	    { RULE_PRINT("statement <- ;\n");}
                | SINGLE_COMMENT_TOK    { RULE_PRINT("statement <- SINGLE_COMMENT\n");}
                ;

expr:           assign_expr			            { RULE_PRINT("expression <- assign_expr\n");}
		        | expr ADD_TOK expr 		    { RULE_PRINT("expression <- expression + expression\n");}
                | expr MINUS_TOK expr 		    { RULE_PRINT("expression <- expression - expression\n");}
                | expr MUL_TOK expr			    { RULE_PRINT("expression <- expression * expression\n");}
                | expr DIV_TOK expr			    { RULE_PRINT("expression <- expression / expression\n");}
                | expr MOD_TOK expr			    { RULE_PRINT("expression <- expression %% expression\n");}
                | expr GREATER_TOK expr		    { RULE_PRINT("expression <- expression > expression\n");}
                | expr GREATER_EQUAL_TOK expr	{ RULE_PRINT("expression <- expression >= expression\n");}
                | expr LESS_TOK expr 		    { RULE_PRINT("expression <- expression < expression\n");}
                | expr LESS_EQUAL_TOK expr		{ RULE_PRINT("expression <- expression <= expression\n");}
                | expr EQUAL_TOK expr           { RULE_PRINT("expression <- expression == expression\n");}
                | expr NOT_EQUAL_TOK expr       { RULE_PRINT("expression <- expression != expression\n");}
                | expr AND_TOK expr             { RULE_PRINT("expression <- expression AND expression\n");}
                | expr OR_TOK expr              { RULE_PRINT("expression <- expression OR expression\n");}
                | term				            { RULE_PRINT("expression <- term\n");}
                ;

term:           LEFT_PARENTHESIS_TOK expr RIGHT_PARENTHESIS_TOK   { RULE_PRINT("term <- ( expression )\n");}
                | MINUS_TOK expr %prec UMINUS_TOK		{ RULE_PRINT("term <- - expression\n");}
                | NOT_TOK expr                          { RULE_PRINT("term <- NOT expression\n");}
                | INCREMENT_TOK lvalue[lval]            { HANDLE_TERM_INC_LVAL($lval); RULE_PRINT("term <- ++ lvalue\n");}
                | lvalue[lval]  INCREMENT_TOK           { HANDLE_TERM_LVAL_INC($lval); RULE_PRINT("term <- lvalue ++\n");}
                | DECREMENT_TOK lvalue[lval]            { HANDLE_TERM_DEC_LVAL($lval); RULE_PRINT("term <- -- lvalue\n");}
                | lvalue[lval]  DECREMENT_TOK           { HANDLE_TERM_LVAL_DEC($lval); RULE_PRINT("term <- lvalue --\n");}
                | primary                               { RULE_PRINT("term <- primary\n");}
                ;       

assign_expr:    lvalue ASSIGN_TOK expr      { RULE_PRINT("assign_expr <- lvalue ASSIGN expression\n");}
                ;

primary:        lvalue         { RULE_PRINT("primary <- lvalue\n");}
                | call         { RULE_PRINT("primary <- call\n");}
                | objectdef    { RULE_PRINT("primary <- object definition\n");}
                | LEFT_PARENTHESIS_TOK funcdef RIGHT_PARENTHESIS_TOK    { RULE_PRINT("primary <- ( function definition )\n");}
                | const        { RULE_PRINT("primary <- const\n");}
                ;

lvalue[lval]:   ID_TOK[id]                      { HANDLE_LVALUE_ID(&$lval, $id); RULE_PRINT("lvalue <- ID\n");}
                | LOCAL_TOK ID_TOK[id]          { HANDLE_LVALUE_LOCAL_ID(&$lval, $id); RULE_PRINT("lvalue <- LOCAL ID\n");}
                | DOUBLE_COLON_TOK ID_TOK[id]   { HANDLE_LVALUE_GLOBAL_ID(&$lval, $id); RULE_PRINT("lvalue <- :: ID\n");}
                | member                        { $lval = NULL; RULE_PRINT("lvalue <- member\n");}
                ;

member:         lvalue DOT_TOK ID_TOK                                   { RULE_PRINT("member <- lvalue . ID\n");}
                | lvalue LEFT_SQUARE_TOK expr RIGHT_SQUARE_TOK          { RULE_PRINT("member <- lvalue [ expr ] \n");}
                | call DOT_TOK ID_TOK                                   { RULE_PRINT("member <- call . ID\n");}
                | call LEFT_SQUARE_TOK expr RIGHT_SQUARE_TOK            { RULE_PRINT("member <- call [ expr ]\n");}
                ;

call:           call LEFT_PARENTHESIS_TOK elist RIGHT_PARENTHESIS_TOK   { RULE_PRINT("call <- call ( elist )\n");}
                | lvalue callsuffix                                     { RULE_PRINT("call <- lvalue callsuffix\n");}
                | LEFT_PARENTHESIS_TOK funcdef RIGHT_PARENTHESIS_TOK LEFT_PARENTHESIS_TOK elist RIGHT_PARENTHESIS_TOK
                    { RULE_PRINT("call <- LEFT_PARENTHESIS_TOK funcdef RIGHT_PARENTHESIS_TOK LEFT_PARENTHESIS_TOK elist RIGHT_PARENTHESIS_TOK\n");}
                ;

callsuffix:     normcall 	    { RULE_PRINT("callsuffix <- normcall\n");}
                | methodcall 	{ RULE_PRINT("callsuffix <- methodcall\n");}
                ;

normcall:       LEFT_PARENTHESIS_TOK elist RIGHT_PARENTHESIS_TOK { RULE_PRINT("normcall <- ( elist )\n");}
                ;

methodcall:     DOUBLE_DOT_TOK ID_TOK LEFT_PARENTHESIS_TOK elist RIGHT_PARENTHESIS_TOK { RULE_PRINT("methodcall <- .. ID ( elist )\n");}
                ;

elist:		    expr COMMA_TOK elist_tail   { RULE_PRINT("elist <- expr , elist_tail\n");}
                | expr                      { RULE_PRINT("elist <- expr\n");}
                | 		                    { RULE_PRINT("elist <- \n");}
                ;

elist_tail: 	expr COMMA_TOK elist_tail   { RULE_PRINT("elist_tail <- expr , elist_tail\n");}
                | expr 			            { RULE_PRINT("elist_tail <- \n");}
                ;

objectdef: 	    LEFT_SQUARE_TOK elist RIGHT_SQUARE_TOK 		    { RULE_PRINT("objectdef <- [ elist ]\n");}
		        | LEFT_SQUARE_TOK indexed RIGHT_SQUARE_TOK 	    { RULE_PRINT("objectdef <- [ indexed ]\n");}
                ;

indexed:        indexedelem indexed_tail            { RULE_PRINT("indexed <- indexedelem indexed_tail\n");}
                ;

indexed_tail: 	COMMA_TOK indexedelem indexed_tail 	{ RULE_PRINT("indexed_tail <- , indexedelem indexed_tail\n");}
                | 				                    { RULE_PRINT("indexed_tail <- \n");}
                ;

indexedelem:    LEFT_BRACKET_TOK expr COLON_TOK expr RIGHT_BRACKET_TOK  { RULE_PRINT("indexedelem <- { expr : expr }\n");}
                ;

block:          LEFT_BRACKET_TOK { scope++; symbolTable_EnterScope(); } stmt_list { scope--;  symbolTable_ExitScope(); } RIGHT_BRACKET_TOK    { RULE_PRINT("block <- { stmt_list }\n"); }
                | LEFT_BRACKET_TOK RIGHT_BRACKET_TOK            { RULE_PRINT("block <- { }\n"); }
                ;

funcdef:        funcdeclare funcparams funcbody { RULE_PRINT("funcdef <- FUNCTION (ID) ( idlist ) block\n"); } 
                ;

funcdeclare:    FUNCTION_TOK ID_TOK             { RULE_PRINT("funcdeclare <- FUNCTION ID\n"); }
                | FUNCTION_TOK                  { RULE_PRINT("funcdeclare <- FUNCTION\n"); }
                ;

funcparams:     LEFT_PARENTHESIS_TOK {scope++;} idlist {scope--;} RIGHT_PARENTHESIS_TOK { RULE_PRINT("funcparams <- ( idlist )\n"); }
                ;

funcblockstart: { uintStack_Push(&loopCounterStack, loopCounter); loopCounter = 0; uintStack_Push(&functionScopeStack, scope); }
                ;

funcblockend:   { loopCounter = uintStack_Pop(&loopCounterStack); uintStack_Pop(&functionScopeStack); }
                ;

funcbody:       funcblockstart block funcblockend   { }
                ;

const:  	    INTEGER_TOK 	    { RULE_PRINT("const <- INTEGER\n");}
                | REAL_TOK		    { RULE_PRINT("const <- REAL\n");}
                | STRING_TOK 	    { RULE_PRINT("const <- STRING_\n");}
                | NIL_TOK 		    { RULE_PRINT("const <- NIL\n");}
                | TRUE_TOK		    { RULE_PRINT("const <- TRUE\n");}
                | FALSE_TOK		    { RULE_PRINT("const <- FALSE\n");}
                ;

idlist: 	    ID_TOK idlist_tail  { RULE_PRINT("idlist <- ID idlist_tail\n");}
                |                   { RULE_PRINT("idlist <- \n");}
                ;

idlist_tail: 	COMMA_TOK ID_TOK idlist_tail    { RULE_PRINT("idlist_tail <- , ID idlist_tail\n");}
                |                               { RULE_PRINT("idlist_tail <- \n");}
                ;

ifstmt:         ifprefix stmt elseprefix stmt   { RULE_PRINT("ifstmt <- ifprefix stmt elsestmt\n");}
                | ifprefix stmt                 { RULE_PRINT("ifstmt <- ifprefix stmt\n");}
                ;

ifprefix :      IF_TOK LEFT_PARENTHESIS_TOK expr RIGHT_PARENTHESIS_TOK { RULE_PRINT("ifprefix <- IF ( expr )\n");}
                ;

elseprefix :    ELSE_TOK { RULE_PRINT("elseprefix <- ELSE\n");}
                ;

loopenter:      { loopCounter++; }
                ;

loopexit:       { assert(loopCounter > 0); loopCounter--; }
                ;

loopstmt:       loopenter stmt loopexit         { RULE_PRINT("loopstmt <- loopenter stmt loopexit\n");}
                ;

whilestmt:      whilestart whileexpr loopstmt   { RULE_PRINT("whilestmt <- while stmt\n"); }
                ;

whilestart:     WHILE_TOK                       { RULE_PRINT("whilestart <- WHILE\n");}
                ;

whileexpr:      LEFT_PARENTHESIS_TOK expr RIGHT_PARENTHESIS_TOK     { RULE_PRINT("whileexpr <- ( expr )\n");}
                ;

forstmt:        forprefix elist RIGHT_PARENTHESIS_TOK loopstmt      { RULE_PRINT("forstmt <- for stmt\n");}
                ;

forprefix:      FOR_TOK LEFT_PARENTHESIS_TOK elist SEMICOLON_TOK expr SEMICOLON_TOK   { RULE_PRINT("forprefix <- FOR ( elist ; expr ; \n");}
                ;

%%

int main(int argc, char** argv){
     if(argc < 2 || argc > 3){
        printf("Usage: %s <input_file> (<output_file>)\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");

    if(yyin == NULL){
        printf("Error: File not found\n");
        return 1;
    }

    char* outputFileName = NULL;

    if(argc == 3){
        outputFileName = malloc(strlen(argv[2]) + 1);
        strcpy(outputFileName, argv[2]);
    }else{
        outputFileName = malloc(strlen("output.bin") + 1);
        strcpy(outputFileName, "output.bin");
        yyout = stdout;
    }

    globalSymbolTable = symbolTable_Init();
    currentSymbolTable = globalSymbolTable;

    symbolTable_Insert("print", LIBFUNC_SYMTYPE);
    symbolTable_Insert("input", LIBFUNC_SYMTYPE);
    symbolTable_Insert("objectmemberkeys", LIBFUNC_SYMTYPE);
    symbolTable_Insert("objecttotalmembers", LIBFUNC_SYMTYPE);
    symbolTable_Insert("objectcopy", LIBFUNC_SYMTYPE);
    symbolTable_Insert("totalarguments", LIBFUNC_SYMTYPE);
    symbolTable_Insert("argument", LIBFUNC_SYMTYPE);
    symbolTable_Insert("typeof", LIBFUNC_SYMTYPE);
    symbolTable_Insert("strtonum", LIBFUNC_SYMTYPE);
    symbolTable_Insert("sqrt", LIBFUNC_SYMTYPE);
    symbolTable_Insert("cos", LIBFUNC_SYMTYPE);
    symbolTable_Insert("sin", LIBFUNC_SYMTYPE);
    
    yyparse();

    // Cleaning Up
    symbolTable_FreeAll();
    uintStack_Clear(&functionScopeStack);
    uintStack_Clear(&loopCounterStack);
    uintStack_Clear(&funcstartJumpStack);
    uintStack_Clear(&tempVarCountStack);
    free(outputFileName);

    return 0;
}
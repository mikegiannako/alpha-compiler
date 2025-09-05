%{
    #include <stdio.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <string.h>
    #include <assert.h>
    #include "../include/lexer.h"
    #include "../include/utils.h"
    #include "../include/generic_stack.h"
	// #include "../include/symtable.h"
    // #include "../include/intermediate.h"
    // #include "../include/parser_rules.h"
    // #include "../include/quad.h"
    // #include "../include/generator.h"
    // #include "../include/vm_code.h"
    // #include "../include/binary.h"

    #ifdef DEBUG
        #define RULE_PRINT(rule) {printf("LINE: %d ", yylineno); printf(rule);}
    #else
        #define RULE_PRINT(rule) {}
    #endif

	/// #define YY_DECL int alpha_yylex (void* ylval)
    
    int yyerror(const char* s);
    int yylex(void);

    extern char* yytext;
    extern FILE* yyin;
    extern FILE* yyout;

    uintStack_ptr functionStack = NULL;
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
	// struct SymbolTableEntry* node_value;
    // struct expr* expr_value;
    // struct stmt* stmt_value;
    unsigned int uintValue;
    // struct call* call_value;
    // struct forLoopPrefix* forLoopPrefix_value;
}

%token <stringValue> STRING ID
%token <numValue> REAL INTEGER

%token SINGLE_COMMENT ASSIGN ADD MINUS MUL DIV MOD EQUAL NOT_EQUAL INCREMENT DECREMENT GREATER GREATER_EQUAL
%token LESS LESS_EQUAL LEFT_BRACKET RIGHT_BRACKET LEFT_SQUARE RIGHT_SQUARE LEFT_PARENTHESIS RIGHT_PARENTHESIS
%token SEMICOLON COMMA COLON DOUBLE_COLON DOT DOUBLE_DOT ELSE

%token IF WHILE FOR FUNCTION RETURN BREAK CONTINUE AND NOT OR LOCAL TRUE FALSE NIL

/* %type <node_value> funcdeclare funcdef
%type <uint_value> funcbody ifprefix elseprefix whilestart whileexpr N M idlist idlist_tail funcparams
%type <expr_value> lvalue member primary assignexpr call objectdef indexed indexed_tail indexedelem expr elist term const elist_tail
%type <call_value> callsuffix normcall methodcall
%type <stmt_value> block stmt stmt_list ifstmt whilestmt forstmt returnstmt loopstmt break continue 
%type <forLoopPrefix_value> forprefix */


/* %right ASSIGN 
%right OR
%left AND
%nonassoc EQUAL NOT_EQUAL
%nonassoc GREATER GREATER_EQUAL LESS LESS_EQUAL
%left ADD MINUS
%left MUL DIV MOD
%right UMINUS NOT PLUSPLUS MINUSMINUS
%left DOT SLEEPING_UP_AND_DOWN_DOT
%left LEFT_BRACKET RIGHT_BRACKET
%left LEFT_PARENTHESIS RIGHT_PARENTHESIS */

%%

program:        {};

/* 
program:        stmt_list                   { RULE_PRINT("program <- stmt_list\n" );}
		        ;

stmt_list:      stmt_list stmt              { RULE_STMTLIST_STMTLIST_STMT(&$$, $1, $2);             RULE_PRINT("statement list <- statement statement list\n");}
                | stmt 		                { RULE_STMTLIST_STMT(&$$, $1); resetTempVarCounter();  RULE_PRINT("statement list <- statement\n");}
                ;   

break:          BREAK SEMICOLON             { RULE_PRINT("break <- BREAK ;\n");}
                ;   

continue:       CONTINUE SEMICOLON          { RULE_PRINT("continue <- CONTINUE ;\n");}
                ;   

returnstmt:     RETURN expr SEMICOLON       { RULE_RETURNSTMT(&$$, $2);         RULE_PRINT("returnstmt <- RETURN expr SEMICOLON\n");}
                | RETURN SEMICOLON          { RULE_RETURNSTMT(&$$, NULL);       RULE_PRINT("returnstmt <- RETURN SEMICOLON\n");}
                ;   

stmt:           expr SEMICOLON { emitIfShortCircuitStmt(&$1); RULE_STMT_GENERIC(&$$);   RULE_PRINT("statement <- expression ;\n");}
                | ifstmt 		            { $$ = $1;                                  RULE_PRINT("statement <- if\n");}
                | whilestmt 		        { RULE_STMT_GENERIC(&$$);                   RULE_PRINT("statement <- while\n");}
                | forstmt 		            { RULE_STMT_GENERIC(&$$);                   RULE_PRINT("statement <- for\n");}
                | returnstmt                { $$ = $1;                                  RULE_PRINT("statement <- return\n");}
                | break                     { RULE_STMT_BREAK(&$$, &$1);                RULE_PRINT("statement <- break ;\n");}
                | continue                  { RULE_STMT_CONTINUE(&$$, &$1);             RULE_PRINT("statement <- continue ;\n");}
                | block 		            { $$ = $1;                                  RULE_PRINT("statement <- block\n");}
                | funcdef 		            { RULE_STMT_GENERIC(&$$);                   RULE_PRINT("statement <- function definition\n");}
                | SEMICOLON 		        { RULE_STMT_GENERIC(&$$);                   RULE_PRINT("statement <- ;\n");}
                | SINGLE_COMMENT 	        { RULE_STMT_GENERIC(&$$);                   RULE_PRINT("statement <- single comment\n");}
                ;

expr:           assignexpr 			        { RULE_EXPR_ASSIGNEXPR(&$$, $1);                RULE_PRINT("expression <- assignment\n");}
		        | expr ADD expr 		    { RULE_EXPR_ARITHOP(&$$, $1, $3, ADD);          RULE_PRINT("expression <- expression ADD expression\n");}
                | expr MINUS expr 		    { RULE_EXPR_ARITHOP(&$$, $1, $3, MINUS);        RULE_PRINT("expression <- expression MINUS expression\n");}
                | expr MUL expr			    { RULE_EXPR_ARITHOP(&$$, $1, $3, MUL);          RULE_PRINT("expression <- expression MUL expression\n");}
                | expr DIV expr			    { RULE_EXPR_ARITHOP(&$$, $1, $3, DIV);          RULE_PRINT("expression <- expression DIV expression\n");}
                | expr MOD expr			    { RULE_EXPR_ARITHOP(&$$, $1, $3, MOD);          RULE_PRINT("expression <- expression MOD expression\n");}
                | expr GREATER expr		    { RULE_EXPR_RELOP(&$$, $1, $3, GREATER);        RULE_PRINT("expression <- expression GREATER expression\n");}
                | expr GREATER_EQUAL expr	{ RULE_EXPR_RELOP(&$$, $1, $3, GREATER_EQUAL);  RULE_PRINT("expression <- expression GREATER_EQUAL expression\n");}
                | expr LESS expr 		    { RULE_EXPR_RELOP(&$$, $1, $3, LESS);           RULE_PRINT("expression <- expression LESS expression\n");}
                | expr LESS_EQUAL expr		{ RULE_EXPR_RELOP(&$$, $1, $3, LESS_EQUAL);     RULE_PRINT("expression <- expression LESS_EQUAL expression\n");}
                | expr EQUAL     { emitIfShortCircuitStmt(&$1); } expr { emitIfShortCircuitStmt(&$4); RULE_EXPR_RELOP(&$$, $1, $4, EQUAL);        RULE_PRINT("expression <- expression EQUAL expression\n");}
                | expr NOT_EQUAL { emitIfShortCircuitStmt(&$1); } expr { emitIfShortCircuitStmt(&$4); RULE_EXPR_RELOP(&$$, $1, $4, NOT_EQUAL);    RULE_PRINT("expression <- expression NOT_EQUAL expression\n");}
                | expr AND       { emitIfNotBool(&$1); }        M expr { emitIfNotBool(&$5);  RULE_EXPR_AND(&$$, $1, $4, $5);    RULE_PRINT("expression <- expression AND expression\n");}
                | expr OR        { emitIfNotBool(&$1); }        M expr { emitIfNotBool(&$5);  RULE_EXPR_OR (&$$, $1, $4, $5);    RULE_PRINT("expression <- expression OR expression\n");}
                | term				        { RULE_EXPR_TERM(&$$, $1);                      RULE_PRINT("expression <- term\n");}
                ;

term:           LEFT_PARENTHESIS expr RIGHT_PARENTHESIS         { RULE_TERM_EXPR(&$$, $2);              RULE_PRINT("term <- ( expression )\n");}
                | MINUS expr %prec UMINUS				        { RULE_TERM_UMINUS_EXPR(&$$, $2);       RULE_PRINT("term <- UMINUS expression\n");}
                | NOT expr                                      { RULE_TERM_NOT_EXPR(&$$, $2);          RULE_PRINT("term <- NOT expression\n");}
                | PLUSPLUS lvalue                               { RULE_TERM_PLUSPLUS_LVALUE(&$$, $2);   RULE_PRINT("term <- ++ lvalue\n");}
                | lvalue PLUSPLUS                               { RULE_TERM_LVALUE_PLUSPLUS(&$$, $1);   RULE_PRINT("term <- lvalue ++\n");}
                | MINUSMINUS lvalue                             { RULE_TERM_MINUSMINUS_LVALUE(&$$, $2); RULE_PRINT("term <- -- lvalue\n");}
                | lvalue MINUSMINUS                             { RULE_TERM_LVALUE_MINUSMINUS(&$$, $1); RULE_PRINT("term <- lvalue --\n");}
                | primary                                       { RULE_TERM_PRIMARY(&$$, $1);           RULE_PRINT("term <- primary\n");}
                ;       

assignexpr:     lvalue ASSIGN expr                              { RULE_ASSIGNEXPR_LVALUE_ASSIGN_EXPR(&$$, $1, $3); RULE_PRINT("assignment <- lvalue ASSIGN expression\n");}
                ;

primary:        lvalue                                          { RULE_PRIMARY_LVALUE(&$$, $1);     RULE_PRINT("primary <- lvalue\n");}
                | call                                          { RULE_PRIMARY_CALL(&$$, $1);       RULE_PRINT("primary <- call\n");}
                | objectdef                                     { RULE_PRIMARY_OBJECTDEF(&$$, $1);  RULE_PRINT("primary <- object definition\n");}
                | LEFT_PARENTHESIS funcdef RIGHT_PARENTHESIS    { RULE_PRIMARY_FUNCDEF(&$$, $2);    RULE_PRINT("primary <- ( function definition )\n");}
                | const                                         { RULE_PRIMARY_CONST(&$$, $1);      RULE_PRINT("primary <- const\n");}
                ;

lvalue:         ID                                              { RULE_LVALUE_ID(&$$, $1);          RULE_PRINT("lvalue <- ID\n");}
                | LOCAL ID                                      { RULE_LVALUE_LOCAL_ID(&$$, $2);    RULE_PRINT("lvalue <- LOCAL ID\n");}
                | DOUBLE_UP_AND_DOWN_DOT ID                     { RULE_LVALUE_GLOBAL_ID(&$$, $2);   RULE_PRINT("lvalue <- :: ID\n");}
                | member                                        { RULE_LVALUE_MEMBER(&$$, $1);      RULE_PRINT("lvalue <- member\n");}
                ;

member:         lvalue DOT ID                                   { RULE_MEMBER_LVALUE_DOT_ID(&$$, $1, $3);   RULE_PRINT("member <- lvalue DOT ID\n");}
                | lvalue LEFT_SQUARE expr RIGHT_SQUARE          { RULE_MEMBER_LVALUE_EXPR(&$$, $1, $3);     RULE_PRINT("member <- lvalue LEFT_SQUARE expr RIGHT_SQUARE \n");}
                | call DOT ID                                   { RULE_MEMBER_CALL_DOT_ID(&$$, $1, $3);     RULE_PRINT("member <- call DOT ID\n");}
                | call LEFT_SQUARE expr RIGHT_SQUARE            { RULE_MEMBER_CALL_EXPR(&$$, $1, $3);       RULE_PRINT("member <- call LEFT_SQUARE expr RIGHT_SQUARE\n");}
                ;

call:           call LEFT_PARENTHESIS elist RIGHT_PARENTHESIS   { RULE_CALL_CALL_ELIST(&$$, $1, $3);        RULE_PRINT("call <- call LEFT_PARENTHESIS elist RIGHT_PARENTHESIS\n");}
                | lvalue callsuffix                             { RULE_CALL_LVALUE_CALLSUFFIX(&$$, $1, $2); RULE_PRINT("call <- lvalue callsuffix\n");}
                | LEFT_PARENTHESIS funcdef RIGHT_PARENTHESIS LEFT_PARENTHESIS elist RIGHT_PARENTHESIS
                                                                { RULE_CALL_FUNCDEF_ELIST(&$$, $2, $5);     RULE_PRINT("call <- LEFT_PARENTHESIS funcdef RIGHT_PARENTHESIS LEFT_PARENTHESIS elist RIGHT_PARENTHESIS\n");}
                ;

callsuffix:     normcall 	    { RULE_CALLSUFFIX_NORMALCALL(&$$, $1); RULE_PRINT("callsuffix <- normcall\n");}
                | methodcall 	{ RULE_CALLSUFFIX_METHODCALL(&$$, $1); RULE_PRINT("callsuffix <- methodcall\n");}
                ;

normcall:       LEFT_PARENTHESIS elist RIGHT_PARENTHESIS { RULE_NORMALCALL_ELIST(&$$, $2); RULE_PRINT("normcall <- LEFT_PARENTHESIS elist RIGHT_PARENTHESIS\n");}
                ;

methodcall:     SLEEPING_UP_AND_DOWN_DOT ID LEFT_PARENTHESIS elist RIGHT_PARENTHESIS { RULE_METHODCALL_ID_ELIST(&$$, $2, $4); RULE_PRINT("methodcall <- SLEEPING_UP_AND_DOWN_DOT ID LEFT_PARENTHESIS elist RIGHT_PARENTHESIS\n");}
                ;

elist:		    expr COMMA { emitIfShortCircuitStmt(&$1);} elist_tail { RULE_ELIST_EXPR_ELIST(&$$, $1, $4);             RULE_PRINT("elist <- expr elist_tail\n");}
                | expr                          { emitIfShortCircuitStmt(&$1); RULE_ELIST_EXPR_ELIST(&$$, $1, NULL);    RULE_PRINT("elist <- expr elist_tail\n");}
                | 		                        { $$ = NULL;                                                            RULE_PRINT("elist <- whatever\n");}
                ;

elist_tail: 	expr COMMA { emitIfShortCircuitStmt(&$1);} elist_tail { RULE_ELIST_EXPR_ELIST(&$$, $1, $4);             RULE_PRINT("elist_tail <- COMMA expr elist_tail\n");}
                | expr 			                { emitIfShortCircuitStmt(&$1); RULE_ELIST_EXPR_ELIST(&$$, $1, NULL);    RULE_PRINT("elist_tail <- whatever\n");}
                ;

objectdef: 	    LEFT_SQUARE elist RIGHT_SQUARE 		    { RULE_OBJECTDEF_ELIST(&$$, $2);    RULE_PRINT("objectdef <- LEFT_SQUARE elist RIGHT_SQUARE\n");}
		        | LEFT_SQUARE indexed RIGHT_SQUARE 	    { RULE_OBEJCTDEF_INDEXED(&$$, $2);  RULE_PRINT("objectdef <- LEFT_SQUARE indexed RIGHT_SQUARE\n");}
                ;

indexed:        indexedelem indexed_tail        { RULE_INDEXED_INDEXEDELEM(&$$, $1, $2);    RULE_PRINT("indexed <- indexedelem indexed_tail\n");}
                ;

indexed_tail: 	COMMA indexedelem indexed_tail 	{ RULE_INDEXED_INDEXEDELEM(&$$, $2, $3);    RULE_PRINT("indexed_tail <- COMMA indexedelem indexed_tail\n");}
                | 				                { $$ = NULL;                                RULE_PRINT("indexed_tail <- whatever\n");}
                ;

indexedelem:    LEFT_BRACKET expr UP_AND_DOWN_DOT { emitIfShortCircuitStmt(&$2); } expr RIGHT_BRACKET  { emitIfShortCircuitStmt(&$5); RULE_INDEXEDELEM_EXPR_COLON_EXPR(&$$, $2, $5); RULE_PRINT("indexedelem <- LEFT_BRACKET expr UP_AND_DOWN_DOT expr RIGHT_BRACKET\n");}
                ;

block:          LEFT_BRACKET { scope++; } stmt_list { SymtableHide(); scope--; resetTempVarCounter(); } RIGHT_BRACKET { $$ = $3; }
                | LEFT_BRACKET { scope++; SymtableHide(); scope--;} RIGHT_BRACKET { RULE_STMT_GENERIC(&$$); } 
                ;

funcdef:        funcdeclare funcparams funcbody { RULE_FUNCDEF(&$$, $1, $2, $3); RULE_PRINT("funcdef <- FUNCTION (ID) LEFT_PARENTHESIS idlist RIGHT_PARENTHESIS block\n"); } 
                ;

funcdeclare:    FUNCTION ID     { RULE_FUNCDECLARE_ID(&$$, $2);                         RULE_PRINT("funcdeclare <- FUNCTION ID\n"); }
                | FUNCTION      { RULE_FUNCDECLARE_ID(&$$, NULL);                       RULE_PRINT("funcdeclare <- FUNCTION\n"); }
                ;

funcparams:     LEFT_PARENTHESIS {scope++;} idlist {scope--;} RIGHT_PARENTHESIS { RULE_FUNCPARAMS_IDLIST(&$$, $3);}
                ;

funcblockstart: { StackPush(&loopCounterStack, loopCounter); loopCounter = 0; }
                ;

funcblockend:   { loopCounter = StackPop(&loopCounterStack); }
                ;

funcbody:       funcblockstart block funcblockend   { RULE_FUNCBODY_BLOCK(&$$, $2);}
                ;

const:  	    INTEGER 	    { RULE_CONST_NUM(&$$, $1);      RULE_PRINT("const <- INTEGER\n");}
                | REAL		    { RULE_CONST_NUM(&$$, $1);      RULE_PRINT("const <- REAL\n");}
                | STRING 	    { RULE_CONST_STRING(&$$, $1);   RULE_PRINT("const <- STRING\n");}
                | NIL 		    { RULE_CONST_NIL(&$$);          RULE_PRINT("const <- NIL\n");}
                | TRUE		    { RULE_CONST_BOOL(&$$, 1);      RULE_PRINT("const <- TRUE\n");}
                | FALSE		    { RULE_CONST_BOOL(&$$, 0);      RULE_PRINT("const <- FALSE\n");}
                ;

idlist: 	    ID idlist_tail  { RULE_IDLIST_ID(&$$, $1, $2);  RULE_PRINT("idlist <- ID & idlist_tail\n");}
                |               { $$ = 0U;                      RULE_PRINT("idlist <- whatever\n");}
                ;

idlist_tail: 	COMMA ID idlist_tail    { RULE_IDLIST_ID(&$$, $2, $3);  RULE_PRINT("idlist_tail <- COMMA ID idlist_tail\n");}
                |                       { $$ = 0U;                      RULE_PRINT("idlist_tail <- whatever\n");}
                ;

ifstmt:         ifprefix stmt elseprefix stmt   { RULE_IFSTMT_IFPREFIX_STMT_ELSEPREFIX_STMT(&$$, $1, $2, $3, $4); RULE_PRINT("ifstmt <- ifprefix stmt elsestmt\n");}
                | ifprefix stmt                 { RULE_IFSTMT_IFPREFIX_STMT(&$$, $1, $2); RULE_PRINT("ifstmt <- ifprefix stmt\n");}
                ;

ifprefix :      IF LEFT_PARENTHESIS expr RIGHT_PARENTHESIS { RULE_IFPREFIX_IF_EXPR(&$$, $3);   RULE_PRINT("ifprefix <- IF LEFT_PARENTHESIS expr RIGHT_PARENTHESIS\n");}
                ;

elseprefix :    ELSE { RULE_ELSEPREFIX_ELSE(&$$); RULE_PRINT("elseprefix <- ELSE\n");}
                ;

loopenter:      { loopCounter++; }
                ;

loopexit:       { assert(loopCounter > 0); loopCounter--; }
                ;

loopstmt:       loopenter stmt loopexit { $$ = $2; RULE_PRINT("loopstmt <- loopenter stmt loopexit\n");}
                ;

whilestmt:      whilestart whileexpr loopstmt { RULE_WHILESTMT_WHILESTART_WHILEEXPR_STMT(&$$, $1, $2, $3); RULE_PRINT("whilestmt <- while stmt\n"); }
                ;

whilestart:     WHILE { RULE_WHILESTART_WHILE(&$$); RULE_PRINT("whilestart <- WHILE\n");}
                ;

whileexpr:      LEFT_PARENTHESIS expr RIGHT_PARENTHESIS { RULE_WHILEEXPR_EXPR(&$$, $2); RULE_PRINT("whileexpr <- ( expr )\n");}
                ;

N:              { $$ = nextQuadLabel(); emit(qop_jump, NULL, NULL, NULL, 0, yylineno); }
                ;

M:              { $$ = nextQuadLabel(); }
                ;

forstmt:        forprefix N elist RIGHT_PARENTHESIS N loopstmt N        { RULE_FORSTMT_FORPREFIX_ELIST_STMT(&$$, $1, $2, $3, $5, $6, $7); RULE_PRINT("forstmt <- for stmt\n");}
                ;

forprefix:      FOR LEFT_PARENTHESIS elist SEMICOLON M expr SEMICOLON   { RULE_FORPREFIX_FOR_ELIST_EXPR(&$$, $3, $5, $6); RULE_PRINT("forprefix <- FOR LEFT_PARENTHESIS elist SEMICOLON M expr SEMICOLON\n");}
                ;

*/

%%

/*
int yyerror (const char * s){
	if ((strstr(s,"$end"))){
		printf("Unexpected reach of the EOF.\n");
	}else{
		printf(RED);
		printf("ERROR : %s --- Line : %d  Token : %s\n", s, yylineno, yylval.string_value);
		printf(RESET);
	}
	return 0;
}

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

    #ifdef DEBUG
        printf("Inserting library functions...\n");
    #endif

    SymtableInit();
    SymtableInsert("print", 0, 0, LIBFUNC);
    SymtableInsert("input", 0, 0, LIBFUNC);
    SymtableInsert("objectmemberkeys", 0, 0, LIBFUNC);
    SymtableInsert("objecttotalmembers", 0, 0, LIBFUNC);
    SymtableInsert("objectcopy", 0, 0, LIBFUNC);
    SymtableInsert("totalarguments", 0, 0, LIBFUNC);
    SymtableInsert("argument", 0, 0, LIBFUNC);
    SymtableInsert("typeof", 0, 0, LIBFUNC);
    SymtableInsert("strtonum", 0, 0, LIBFUNC);
    SymtableInsert("sqrt", 0, 0, LIBFUNC);
    SymtableInsert("cos", 0, 0, LIBFUNC);
    SymtableInsert("sin", 0, 0, LIBFUNC);

    #ifdef DEBUG
        printf("Parsing...\n");
    #endif
    
    yyparse();

    #ifdef DEBUG
        printf("\n\nPrinting Symbol Table...\n");
        SymtablePrint();
        printf("\n\n");
        printf("\nPrinting Quads...\n");
    #endif
    
    printQuads();

    printf("\n\b");
    #ifdef DEBUG
        printf("\nGenerating Instructions...\n");
    #endif

    generateInstructions();

    #ifdef DEBUG
        printf("Printing Instructions...\n");
    #endif

    printInstructions();

    writeBinaryFile(outputFileName);

    SymtableDestroy();
} */

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
    
    yyparse();

    return 0;
}
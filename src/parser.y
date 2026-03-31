%{
    #include <stdio.h>
	#include <string.h>
    #include <stdbool.h>
    #include <assert.h>
    #include "../include/lexer.h"
    #include "../include/utils.h"
    #include "../include/generic_stack.h"
	#include "../include/symtable.h"
    #include "../include/rule_handlers.h"
    #include "../include/intermediate.h"

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
    const char* sourceFileName = "unknown";
%}

%start program

%define parse.error verbose
%define parse.lac full

%union{
	const char* stringValue;
	float numValue;
    unsigned int uintValue;
    struct symbolTableEntry* symbolValue;
    struct call* callValue;
}

%token <stringValue> STRING_TOK "string" ID_TOK "identifier"
%token <numValue> REAL_TOK "real number" INTEGER_TOK "integer"

%token SINGLE_COMMENT_TOK "comment"
%token ASSIGN_TOK "="
%token ADD_TOK "+" MINUS_TOK "-" MUL_TOK "*" DIV_TOK "/" MOD_TOK "%"
%token EQUAL_TOK "==" NOT_EQUAL_TOK "!="
%token INCREMENT_TOK "++" DECREMENT_TOK "--"
%token GREATER_TOK ">" GREATER_EQUAL_TOK ">=" LESS_TOK "<" LESS_EQUAL_TOK "<="
%token LEFT_BRACKET_TOK "{" RIGHT_BRACKET_TOK "}"
%token LEFT_SQUARE_TOK "[" RIGHT_SQUARE_TOK "]"
%token LEFT_PARENTHESIS_TOK "(" RIGHT_PARENTHESIS_TOK ")"
%token SEMICOLON_TOK ";" COMMA_TOK "," COLON_TOK ":" DOUBLE_COLON_TOK "::" DOT_TOK "." DOUBLE_DOT_TOK ".."
%token IF_TOK "if" ELSE_TOK "else" WHILE_TOK "while" FOR_TOK "for"
%token FUNCTION_TOK "function" RETURN_TOK "return"
%token BREAK_TOK "break" CONTINUE_TOK "continue"
%token AND_TOK "and" NOT_TOK "not" OR_TOK "or"
%token LOCAL_TOK "local" TRUE_TOK "true" FALSE_TOK "false" NIL_TOK "nil"
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

%type<symbolValue> lvalue funcdeclare idlist
%type<callValue> methodcall normcall callsuffix

%%


program:        stmt_list               { RULE_PRINT("program <- stmt_list\n" );}
                |
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

member:         lvalue[lval] DOT_TOK ID_TOK[id]                         { HANDLE_MEMBER_LVALUE_ID($lval, $id); RULE_PRINT("member <- lvalue . ID\n");}
                | lvalue[lval] LEFT_SQUARE_TOK expr RIGHT_SQUARE_TOK    { HANDLE_MEMBER_LVALUE_EXPR($lval); RULE_PRINT("member <- lvalue [ expr ] \n");}
                | call DOT_TOK ID_TOK                                   { RULE_PRINT("member <- call . ID\n");}
                | call LEFT_SQUARE_TOK expr RIGHT_SQUARE_TOK            { RULE_PRINT("member <- call [ expr ]\n");}
                ;

call:           call LEFT_PARENTHESIS_TOK elist RIGHT_PARENTHESIS_TOK   { RULE_PRINT("call <- call ( elist )\n");}
                | lvalue[lval] callsuffix[suffix]                       { HANDLE_CALL_LVALUE_CALLSUFFIX($lval, $suffix); RULE_PRINT("call <- lvalue callsuffix\n");}
                | LEFT_PARENTHESIS_TOK funcdef RIGHT_PARENTHESIS_TOK LEFT_PARENTHESIS_TOK elist RIGHT_PARENTHESIS_TOK
                    { RULE_PRINT("call <- LEFT_PARENTHESIS_TOK funcdef RIGHT_PARENTHESIS_TOK LEFT_PARENTHESIS_TOK elist RIGHT_PARENTHESIS_TOK\n");}
                ;

callsuffix:     normcall[call] 	    { $$ = $call; RULE_PRINT("callsuffix <- normcall\n");}
                | methodcall[call]	{ $$ = $call; RULE_PRINT("callsuffix <- methodcall\n");}
                ;

normcall:       LEFT_PARENTHESIS_TOK elist RIGHT_PARENTHESIS_TOK { $$ = createCallValue(false, NULL); RULE_PRINT("normcall <- ( elist )\n");}
                ;

methodcall:     DOUBLE_DOT_TOK ID_TOK[id] LEFT_PARENTHESIS_TOK elist RIGHT_PARENTHESIS_TOK { $$ = createCallValue(true, $id); RULE_PRINT("methodcall <- .. ID ( elist )\n");}
                ;

elist:		    elist COMMA_TOK expr        { RULE_PRINT("elist <- elist , expr\n");}
                | expr                      { RULE_PRINT("elist <- expr\n");}
                | 		                    { RULE_PRINT("elist <- \n");}
                ;

objectdef: 	    LEFT_SQUARE_TOK elist RIGHT_SQUARE_TOK 		    { RULE_PRINT("objectdef <- [ elist ]\n");}
		        | LEFT_SQUARE_TOK indexed RIGHT_SQUARE_TOK 	    { RULE_PRINT("objectdef <- [ indexed ]\n");}
                ;

indexed:        indexed COMMA_TOK indexedelem        { RULE_PRINT("indexed <- indexed , indexedelem\n");}
                | indexedelem                        { RULE_PRINT("indexed <- indexedelem\n");}
                ;

indexedelem:    LEFT_BRACKET_TOK expr COLON_TOK expr RIGHT_BRACKET_TOK  { RULE_PRINT("indexedelem <- { expr : expr }\n");}
                ;

block:          LEFT_BRACKET_TOK { scope++; symbolTable_EnterScope(); } stmt_list { scope--;  symbolTable_ExitScope(true); } RIGHT_BRACKET_TOK    { RULE_PRINT("block <- { stmt_list }\n"); }
                | LEFT_BRACKET_TOK RIGHT_BRACKET_TOK            { RULE_PRINT("block <- { }\n"); }
                ;

funcdef:        funcdeclare funcparams funcbody { RULE_PRINT("funcdef <- FUNCTION ID ( idlist ) block\n"); } 
                ;

funcdeclare[funcdecl]:  FUNCTION_TOK ID_TOK[id]         { HANDLE_FUNCDECLARE_ID(&$funcdecl, $id); RULE_PRINT("funcdeclare <- FUNCTION ID\n"); }
                        | FUNCTION_TOK                  { HANDLE_FUNCDECLARE_ANON(&$funcdecl); RULE_PRINT("funcdeclare <- FUNCTION\n"); }
                        ;

funcparams:     LEFT_PARENTHESIS_TOK  { scope++; symbolTable_EnterScope(); } idlist { scope--;  symbolTable_ExitScope(false); } RIGHT_PARENTHESIS_TOK { RULE_PRINT("funcparams <- ( idlist )\n"); }
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

idlist[list]:   idlist[tail] COMMA_TOK ID_TOK[id]   { HANDLE_IDLIST(&$list, $id, $tail); RULE_PRINT("idlist <- idlist , ID\n");}
                | ID_TOK[id]                        { HANDLE_IDLIST(&$list, $id, NULL); RULE_PRINT("idlist <- ID\n");}
                |                                   { $list = NULL; RULE_PRINT("idlist <- \n");}
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

    sourceFileName = argv[1];
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

    symbolTable_Print();

    // Cleaning Up
    symbolTable_FreeAll();
    uintStack_Clear(&functionScopeStack);
    uintStack_Clear(&loopCounterStack);
    uintStack_Clear(&funcstartJumpStack);
    uintStack_Clear(&tempVarCountStack);
    free(outputFileName);

    return 0;
}
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
    #include "../include/intermediate_rule_types.h"
    #include "../include/quad.h"
    #include "../include/config.h"
    #include "../include/generator.h"
    #include "../include/instruction.h"
    #include "../include/vm_code.h"
    #include "../include/binary.h"

    #ifdef DEBUG
        #define RULE_PRINT(rule) {fprintf(stderr, "LINE: %d ", yylineno); fprintf(stderr, rule);}
    #else
        #define RULE_PRINT(rule) {}
    #endif

    #define EMPTY_IDLIST 0U

	// #define YY_DECL int alpha_yylex (void* ylval)
    
    int yyerror(const char* s);
    int yylex(void);

    extern char* yytext;
    extern FILE* yyin;
    extern FILE* yyout;

    UIntStack_ptr functionScopeStack = NULL;
    UIntStack_ptr loopCounterStack = NULL;
    UIntStack_ptr funcstartJumpStack = NULL;
    UIntStack_ptr tempVarCountStack = NULL;
    UIntStack_ptr scopeOffsetStack = NULL;
    unsigned int loopCounter = 0;
    unsigned int scope = 0;
    const char* sourceFileName = "unknown";
%}

%start program

%define parse.error verbose
%define parse.lac full

%expect 1

%union{
	const char* stringValue;
	float numValue;
    unsigned int uintValue;
    struct SymbolTableEntry* symbolValue;
    struct Call* callValue;
    struct Expr* exprValue;
    struct Stmt* stmtValue;
    struct ForLoopPrefix* forLoopPrefixValue;

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

%type<symbolValue> funcdeclare funcname funcdef
%type<callValue> methodcall normcall callsuffix
%type<uintValue> funcbody ifprefix elseprefix whilestart whileexpr funcparams idlist M N
%type<stmtValue> block stmt stmt_list ifstmt whilestmt forstmt returnstmt loopstmt break continue program 
%type<exprValue> const expr elist term assign_expr primary call objectdef lvalue member indexed indexedelem
%type<forLoopPrefixValue> forprefix

%%


program:        stmt_list               { $$ = $1;      RULE_PRINT("program <- stmt_list\n" );}
                |                       { $$ = NULL;    RULE_PRINT("program <- \n" );}
		        ;

stmt_list:      stmt_list { resetTempVarCounter(); } stmt { HANDLE_STMTLIST(&$$, $1, $3); RULE_PRINT("statement list <- statement statement list\n");}
                | stmt 		            { $$ = $1; RULE_PRINT("statement list <- statement\n");}
                ;   

break:          BREAK_TOK SEMICOLON_TOK         { HANDLE_BREAK(&$$); RULE_PRINT("break <- BREAK ;\n");}
                ;   

continue:       CONTINUE_TOK SEMICOLON_TOK      { HANDLE_CONTINUE(&$$); RULE_PRINT("continue <- CONTINUE ;\n");}
                ;   

returnstmt:     RETURN_TOK expr SEMICOLON_TOK   { HANDLE_RETURN(&$$, $2); RULE_PRINT("returnstmt <- RETURN expression ;\n");}
                | RETURN_TOK SEMICOLON_TOK      { HANDLE_RETURN(&$$, NULL); RULE_PRINT("returnstmt <- RETURN ;\n");}
                ;   

stmt:           expr SEMICOLON_TOK      { emitIfShortCircuitStmt(&$1); noReturnCheck_CancelIfCallResult($1); HANDLE_STMT_GENERIC(&$$); RULE_PRINT("statement <- expression ;\n");}
                | ifstmt 		        { $$ = $1; RULE_PRINT("statement <- if\n");}
                | whilestmt 		    { $$ = $1; RULE_PRINT("statement <- while\n");}
                | forstmt 		        { $$ = $1; RULE_PRINT("statement <- for\n");}
                | returnstmt            { $$ = $1; RULE_PRINT("statement <- return\n");}
                | break                 { $$ = $1; RULE_PRINT("statement <- break ;\n");}
                | continue              { $$ = $1; RULE_PRINT("statement <- continue ;\n");}
                | block 		        { $$ = $1; RULE_PRINT("statement <- block\n");}
                | funcdef 		        { HANDLE_STMT_GENERIC(&$$); RULE_PRINT("statement <- function definition\n");}
                | SEMICOLON_TOK 	    { HANDLE_STMT_GENERIC(&$$); RULE_PRINT("statement <- ;\n");}
                | SINGLE_COMMENT_TOK    { HANDLE_STMT_GENERIC(&$$); RULE_PRINT("statement <- SINGLE_COMMENT\n");}
                ;

expr:           assign_expr			                        { $$ = $1; RULE_PRINT("expression <- assign_expr\n");}
		        | expr[expr1] ADD_TOK expr[expr2] 		    { RULE_EXPR_ARITHOP(&$$, $expr1, $expr2, ADD_TOK);          RULE_PRINT("expression <- expression + expression\n"); }
                | expr[expr1] MINUS_TOK expr[expr2] 		{ RULE_EXPR_ARITHOP(&$$, $expr1, $expr2, MINUS_TOK);        RULE_PRINT("expression <- expression - expression\n"); }
                | expr[expr1] MUL_TOK expr[expr2]			{ RULE_EXPR_ARITHOP(&$$, $expr1, $expr2, MUL_TOK);          RULE_PRINT("expression <- expression * expression\n"); }
                | expr[expr1] DIV_TOK expr[expr2]			{ RULE_EXPR_ARITHOP(&$$, $expr1, $expr2, DIV_TOK);          RULE_PRINT("expression <- expression / expression\n"); }
                | expr[expr1] MOD_TOK expr[expr2]			{ RULE_EXPR_ARITHOP(&$$, $expr1, $expr2, MOD_TOK);          RULE_PRINT("expression <- expression %% expression\n");}
                | expr[expr1] GREATER_TOK expr[expr2]		{ RULE_EXPR_RELOP(&$$, $expr1, $expr2, GREATER_TOK);        RULE_PRINT("expression <- expression > expression\n");}
                | expr[expr1] GREATER_EQUAL_TOK expr[expr2]	{ RULE_EXPR_RELOP(&$$, $expr1, $expr2, GREATER_EQUAL_TOK);  RULE_PRINT("expression <- expression >= expression\n");}
                | expr[expr1] LESS_TOK expr[expr2] 		    { RULE_EXPR_RELOP(&$$, $expr1, $expr2, LESS_TOK);           RULE_PRINT("expression <- expression < expression\n");}
                | expr[expr1] LESS_EQUAL_TOK expr[expr2]	{ RULE_EXPR_RELOP(&$$, $expr1, $expr2, LESS_EQUAL_TOK);     RULE_PRINT("expression <- expression <= expression\n");}
                | expr[expr1] EQUAL_TOK { emitIfShortCircuitStmt(&$expr1);}      expr[expr2] { emitIfShortCircuitStmt(&$expr2); RULE_EXPR_RELOP(&$$, $expr1, $expr2, EQUAL_TOK);          RULE_PRINT("expression <- expression == expression\n");}
                | expr[expr1] NOT_EQUAL_TOK  { emitIfShortCircuitStmt(&$expr1);} expr[expr2] { emitIfShortCircuitStmt(&$expr2); RULE_EXPR_RELOP(&$$, $expr1, $expr2, NOT_EQUAL_TOK);      RULE_PRINT("expression <- expression != expression\n");}
                | expr[expr1] AND_TOK   { emitIfNotBool(&$expr1); }  M[marker]   expr[expr2] { emitIfNotBool(&$expr2); RULE_EXPR_AND(&$$, $expr1, $marker, $expr2);  RULE_PRINT("expression <- expression AND expression\n");}
                | expr[expr1] OR_TOK    { emitIfNotBool(&$expr1); }  M[marker]   expr[expr2] { emitIfNotBool(&$expr2); RULE_EXPR_OR (&$$, $expr1, $marker, $expr2);  RULE_PRINT("expression <- expression OR expression\n");}
                | term				                        { $$ = $1; RULE_PRINT("expression <- term\n");}
                ;

term:           LEFT_PARENTHESIS_TOK expr RIGHT_PARENTHESIS_TOK { $$ = $2; RULE_PRINT("term <- ( expression )\n");}
                | MINUS_TOK expr[expr1] %prec UMINUS_TOK        { HANDLE_TERM_UMINUS_EXPR(&$$, $expr1); RULE_PRINT("term <- - expression\n");}
                | NOT_TOK expr[expr1]                           { HANDLE_TERM_NOT_EXPR(&$$, $expr1);    RULE_PRINT("term <- NOT expression\n");}
                | INCREMENT_TOK lvalue[lval]                    { HANDLE_TERM_INC_LVAL(&$term, $lval);  RULE_PRINT("term <- ++ lvalue\n");}
                | lvalue[lval]  INCREMENT_TOK                   { HANDLE_TERM_LVAL_INC(&$term, $lval);  RULE_PRINT("term <- lvalue ++\n");}
                | DECREMENT_TOK lvalue[lval]                    { HANDLE_TERM_DEC_LVAL(&$term, $lval);  RULE_PRINT("term <- -- lvalue\n");}
                | lvalue[lval]  DECREMENT_TOK                   { HANDLE_TERM_LVAL_DEC(&$term, $lval);  RULE_PRINT("term <- lvalue --\n");}
                | primary                                       { $$ = $1;                              RULE_PRINT("term <- primary\n");}
                ;       

assign_expr[assign]:    lvalue[lval] ASSIGN_TOK expr[expr1]      { HANDLE_ASSIGNEXPR(&$assign, $lval, $expr1); RULE_PRINT("assign_expr <- lvalue ASSIGN expression\n"); }
                        ;

primary[prim]:  lvalue[lval]                                                { $prim = emitIfTableItem($lval); RULE_PRINT("primary <- lvalue\n");}
                | call                                                      { $prim = $1; RULE_PRINT("primary <- call\n");}
                | objectdef                                                 { $prim = $1; RULE_PRINT("primary <- object definition\n");}
                | LEFT_PARENTHESIS_TOK funcdef[func] RIGHT_PARENTHESIS_TOK  { HANDLE_PRIMARY_FUNCDEF(&$prim, $func); RULE_PRINT("primary <- ( function definition )\n");}
                | const                                                     { $prim = $1; RULE_PRINT("primary <- const\n");}
                ;

lvalue[lval]:   ID_TOK[id]                      { HANDLE_LVALUE_ID(&$lval, $id); RULE_PRINT("lvalue <- ID\n");}
                | LOCAL_TOK ID_TOK[id]          { HANDLE_LVALUE_LOCAL_ID(&$lval, $id); RULE_PRINT("lvalue <- LOCAL ID\n");}
                | DOUBLE_COLON_TOK ID_TOK[id]   { HANDLE_LVALUE_GLOBAL_ID(&$lval, $id); RULE_PRINT("lvalue <- :: ID\n");}
                | member                        { $lval = $1; RULE_PRINT("lvalue <- member\n");}
                ;

member:         lvalue[lval] DOT_TOK ID_TOK[id]                             { HANDLE_MEMBER_DOT(&$$, $lval, $id); RULE_PRINT("member <- lvalue . ID\n");}
                | lvalue[lval] LEFT_SQUARE_TOK expr[expr1] RIGHT_SQUARE_TOK { HANDLE_MEMBER_BRACKET(&$$, $lval, $expr1); RULE_PRINT("member <- lvalue [ expr ] \n");}
                | call DOT_TOK ID_TOK[id]                                   { HANDLE_MEMBER_DOT(&$$, $1, $id); RULE_PRINT("member <- call . ID\n");}
                | call LEFT_SQUARE_TOK expr[expr1] RIGHT_SQUARE_TOK         { HANDLE_MEMBER_BRACKET(&$$, $1, $expr1); RULE_PRINT("member <- call [ expr ]\n");}
                ;

call:           call[call1] LEFT_PARENTHESIS_TOK elist[exprs] RIGHT_PARENTHESIS_TOK   { $$ = makeCall($call1, $exprs); RULE_PRINT("call <- call ( elist )\n");}
                | lvalue[lval] callsuffix[suffix]                       { HANDLE_CALL_LVALUE_CALLSUFFIX(&$$, $lval, $suffix); RULE_PRINT("call <- lvalue callsuffix\n");}
                | LEFT_PARENTHESIS_TOK funcdef[func] RIGHT_PARENTHESIS_TOK LEFT_PARENTHESIS_TOK elist[exprs] RIGHT_PARENTHESIS_TOK { HANDLE_CALL_FUNCDEF_ELIST(&$$, $func, $exprs); RULE_PRINT("call <- LEFT_PARENTHESIS_TOK funcdef RIGHT_PARENTHESIS_TOK LEFT_PARENTHESIS_TOK elist RIGHT_PARENTHESIS_TOK\n");}
                ;

callsuffix:     normcall[call] 	    { $$ = $call; RULE_PRINT("callsuffix <- normcall\n");}
                | methodcall[call]	{ $$ = $call; RULE_PRINT("callsuffix <- methodcall\n");}
                ;

normcall:       LEFT_PARENTHESIS_TOK elist[exprs] RIGHT_PARENTHESIS_TOK { $$ = createCallValue(false, NULL, $exprs); RULE_PRINT("normcall <- ( elist )\n");}
                ;

methodcall:     DOUBLE_DOT_TOK ID_TOK[id] LEFT_PARENTHESIS_TOK elist[exprs] RIGHT_PARENTHESIS_TOK { $$ = createCallValue(true, $id, $exprs); RULE_PRINT("methodcall <- .. ID ( elist )\n");}
                ;

elist:		    elist[list] COMMA_TOK expr[expr1]   { emitIfShortCircuitStmt(&$expr1); $expr1->next = $list; $$ = $expr1; RULE_PRINT("elist <- elist , expr\n");}
                | expr[expr1]                       { emitIfShortCircuitStmt(&$expr1); $$ = $expr1;   RULE_PRINT("elist <- expr\n");}
                | 		                            { $$ = NULL; RULE_PRINT("elist <- \n");}
                ;

objectdef[obj]: LEFT_SQUARE_TOK elist[list] RIGHT_SQUARE_TOK 	    { HANDLE_OBJECTDEF_ELIST(&$obj, $list); RULE_PRINT("objectdef <- [ elist ]\n");}
		        | LEFT_SQUARE_TOK indexed[list] RIGHT_SQUARE_TOK    { HANDLE_OBEJCTDEF_INDEXED(&$obj, $list); RULE_PRINT("objectdef <- [ indexed ]\n");}
                ;

indexed:        indexed[list] COMMA_TOK indexedelem[elem]   { $elem->next = $list; $$ = $elem; RULE_PRINT("indexed <- indexed , indexedelem\n");}
                | indexedelem[elem]                         { $$ = $elem; RULE_PRINT("indexed <- indexedelem\n");}
                ;

indexedelem:    LEFT_BRACKET_TOK expr[key] { emitIfShortCircuitStmt(&$key); } COLON_TOK expr[value] RIGHT_BRACKET_TOK { emitIfShortCircuitStmt(&$value); $value->index = $key; $$ = $value; RULE_PRINT("indexedelem <- { expr : expr }\n");}
                ;

block:          LEFT_BRACKET_TOK { scope++; symbolTable_EnterScope(); } stmt_list { scope--;  symbolTable_ExitScope(true); } RIGHT_BRACKET_TOK { $$ = $3; RULE_PRINT("block <- { stmt_list }\n"); }
                | LEFT_BRACKET_TOK RIGHT_BRACKET_TOK    { $$ = NULL; symbolTable_EnterScope(); symbolTable_ExitScope(true); RULE_PRINT("block <- { }\n"); }
                ;

funcdef[func]:  funcdeclare[funcdecl] funcparams[params] funcbody[body] { RULE_FUNCDEF(&$func, $funcdecl, $params, $body); RULE_PRINT("funcdef <- FUNCTION ID ( idlist ) block\n"); } 
                ;

funcname[name]:     ID_TOK[id]  { HANDLE_FUNCNAME_ID(&$name, $id); RULE_PRINT("funcname <- ID\n"); }
                    |           { HANDLE_FUNCNAME_ANON(&$name); RULE_PRINT("funcname <-\n"); }
                    ;

funcdeclare[funcdecl]:  FUNCTION_TOK funcname[id]       { HANDLE_FUNCDECLARE_FUNCNAME(&$funcdecl, $id); RULE_PRINT("funcdeclare <- FUNCTION funcname\n")}

funcparams[params]:     LEFT_PARENTHESIS_TOK  { scope++; symbolTable_EnterScope(); } idlist[ids] { scope--;  symbolTable_ExitScope(false); } RIGHT_PARENTHESIS_TOK { HANDLE_FUNCPARAMS(&$params, $ids); RULE_PRINT("funcparams <- ( idlist )\n"); }
                        ;

funcblockstart: { uintStack_Push(&loopCounterStack, loopCounter); loopCounter = 0; uintStack_Push(&functionScopeStack, scope); }
                ;

funcblockend:   { loopCounter = uintStack_Pop(&loopCounterStack); uintStack_Pop(&functionScopeStack); }
                ;

funcbody[body]: funcblockstart block[blck] funcblockend   { HANDLE_FUNCBODY(&$body, $blck); RULE_PRINT("funcbody <- body\n");}
                ;

const:  	    INTEGER_TOK[num] 	{ $$ = expr_NewConstNum($num);       RULE_PRINT("const <- INTEGER\n");}
                | REAL_TOK[realnum]	{ $$ = expr_NewConstNum($realnum);   RULE_PRINT("const <- REAL\n");}
                | STRING_TOK[str] 	{ $$ = expr_NewConstString($str);    RULE_PRINT("const <- STRING_\n");}
                | NIL_TOK 		    { $$ = expr_New(NIL_EXPRTYPE);       RULE_PRINT("const <- NIL\n");}
                | TRUE_TOK		    { $$ = expr_NewConstBool(true);      RULE_PRINT("const <- TRUE\n");}
                | FALSE_TOK		    { $$ = expr_NewConstBool(false);     RULE_PRINT("const <- FALSE\n");}
                ;

idlist[list]:   idlist[tail] COMMA_TOK ID_TOK[id]   { HANDLE_IDLIST(&$list, $id, $tail); RULE_PRINT("idlist <- idlist , ID\n");}
                | ID_TOK[id]                        { HANDLE_IDLIST(&$list, $id, EMPTY_IDLIST); RULE_PRINT("idlist <- ID\n");}
                |                                   { $list = EMPTY_IDLIST; RULE_PRINT("idlist <- \n");}
                ;

ifstmt[if]:     ifprefix[prefix1] stmt[stmt1] elseprefix[prefix2] stmt[stmt2]   { HANDLE_IFSTMT_IF_ELSE_STMT(&$if, $prefix1, $stmt1, $prefix2, $stmt2); RULE_PRINT("ifstmt <- ifprefix stmt elsestmt\n");}
                | ifprefix[prefix] stmt[stmt1]  { HANDLE_IFSTMT_IFPREFIX_STMT(&$if, $prefix, $stmt1); RULE_PRINT("ifstmt <- ifprefix stmt\n");}
                ;

ifprefix[prefix]:   IF_TOK LEFT_PARENTHESIS_TOK expr RIGHT_PARENTHESIS_TOK { HANDLE_IFPREFIX_IF_EXPR(&$prefix, $3); RULE_PRINT("ifprefix <- IF ( expr )\n");}
                    ;

elseprefix[prefix]: ELSE_TOK { HANDLE_ELSEPREFIX_ELSE(&$prefix); RULE_PRINT("elseprefix <- ELSE\n");}
                    ;

loopenter:      { loopCounter++; }
                ;

loopexit:       { assert(loopCounter > 0); loopCounter--; }
                ;

loopstmt:       loopenter stmt loopexit         { $$ = $2; RULE_PRINT("loopstmt <- loopenter stmt loopexit\n");}
                ;

whilestmt:      whilestart[start] whileexpr[cond] loopstmt[body]   { HANDLE_WHILESTMT(&$$, $start, $cond, $body); RULE_PRINT("whilestmt <- while stmt\n"); }
                ;

whilestart[start]:  WHILE_TOK                   { $start = quad_NextLabel(); RULE_PRINT("whilestart <- WHILE\n");}
                    ;

whileexpr:      LEFT_PARENTHESIS_TOK expr[cond] RIGHT_PARENTHESIS_TOK     { HANDLE_WHILEEXPR(&$$, $cond); RULE_PRINT("whileexpr <- ( expr )\n");}
                ;


N:          { $$ = quad_NextLabel(); quad_Emit(JUMP_QUADOP, NULL, NULL, NULL, NO_LABEL); }
            ;

M:          { $$ = quad_NextLabel(); }
            ;

forstmt[for]:   forprefix[prefix] N[jump1] elist[exprs] RIGHT_PARENTHESIS_TOK N[jump2] loopstmt[stmt] N[jump3]  { RULE_FORSTMT_FORPREFIX_ELIST_STMT(&$for, $prefix, $jump1, $exprs, $jump2, $stmt, $jump3); RULE_PRINT("forstmt <- for stmt\n");}
                ;

forprefix[prefix]:  FOR_TOK LEFT_PARENTHESIS_TOK elist[exprs] SEMICOLON_TOK M[marker] expr[expr1] SEMICOLON_TOK { RULE_FORPREFIX_FOR_ELIST_EXPR(&$prefix, $exprs, $marker, $expr1 ); RULE_PRINT("forprefix <- FOR ( elist ; expr ; \n");}
                    ;




%%

int main(int argc, char** argv){
    const char* inputFileName = NULL;
    const char* outputArg = NULL;

    // Split argv into recognised toggle flags and the positional <input> [output].
    for(int i = 1; i < argc; i++){
        if(config_TryParseFlag(argv[i])) continue;
        if(argv[i][0] == '-' && argv[i][1] == '-'){
            printf("Error: unknown flag '%s'\n", argv[i]);
            return 1;
        }
        if(inputFileName == NULL)      inputFileName = argv[i];
        else if(outputArg == NULL)     outputArg = argv[i];
        else { printf("Error: too many arguments\n"); return 1; }
    }

    if(inputFileName == NULL){
        printf("Usage: %s [flags] <input_file> (<output_file>)\n", argv[0]);
        printf("Flags: --[no-]funcstart-jump --[no-]return-jump --[no-]short-circuit-backpatch\n");
        return 1;
    }

    sourceFileName = inputFileName;
    yyin = fopen(inputFileName, "r");

    if(yyin == NULL){
        printf("Error: File not found\n");
        return 1;
    }

    char* outputFileName = NULL;

    if(outputArg != NULL){
        outputFileName = safeStrDup(outputArg, "copying output filename arg to var");
    }else{
        outputFileName = safeStrDup("output.bin", "copying \"output.bin\" to var");
        yyout = stdout;
    }

    globalSymbolTable = symbolTable_Init();
    currentSymbolTable = globalSymbolTable;

    symbolTable_Insert("print", LIB_FUNC_SYMTYPE);
    symbolTable_Insert("input", LIB_FUNC_SYMTYPE);
    symbolTable_Insert("objectmemberkeys", LIB_FUNC_SYMTYPE);
    symbolTable_Insert("objecttotalmembers", LIB_FUNC_SYMTYPE);
    symbolTable_Insert("objectcopy", LIB_FUNC_SYMTYPE);
    symbolTable_Insert("totalarguments", LIB_FUNC_SYMTYPE);
    symbolTable_Insert("argument", LIB_FUNC_SYMTYPE);
    symbolTable_Insert("typeof", LIB_FUNC_SYMTYPE);
    symbolTable_Insert("strtonum", LIB_FUNC_SYMTYPE);
    symbolTable_Insert("sqrt", LIB_FUNC_SYMTYPE);
    symbolTable_Insert("cos", LIB_FUNC_SYMTYPE);
    symbolTable_Insert("sin", LIB_FUNC_SYMTYPE);

    #ifdef DEBUG
        printf("Parsing...\n");
    #endif
    
    yyparse();

    // Best-effort warning: a no-return user function whose result is used as a value.
    noReturnCheck_ReportAll();

    #ifdef DEBUG
        printf("\n\nPrinting Symbol Table...\n");
        symbolTable_Print();

        printf("\n\n");
        printf("\nPrinting Quads...\n");
        quad_PrintAll();
        printf("\n\nGenerating target code...\n");
    #endif

    generateInstructions();

    #ifdef DEBUG
        printInstructions();
    #endif

    writeBinaryFile(outputFileName);

    // Cleaning Up. The constant tables and instructions must be freed after the
    // binary is written and the tables printed above (libFuncs borrows symbol
    // table names, so free it before symbolTable_FreeAll()).
    vmCode_FreeAll();
    instructions_FreeAll();
    exprArena_FreeAll();
    callArena_FreeAll();
    stmtArena_FreeAll();
    forPrefixArena_FreeAll();
    symbolTable_FreeAll();
    lexer_FreeTrackedStrings();
    uintStack_Clear(&functionScopeStack);
    uintStack_Clear(&loopCounterStack);
    uintStack_Clear(&funcstartJumpStack);
    uintStack_Clear(&tempVarCountStack);
    safeFree(&outputFileName, "freeing compiler output filename");

    return 0;
}
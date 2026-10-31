%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symtab.h"

extern int yylex();
extern int yylineno;
extern char *yytext;
extern FILE *yyin;
extern int lexical_errors;

void yyerror(const char *s);

ASTNode *ast_root = NULL;
int syntax_errors = 0;
extern int semantic_errors;

/* Helper to append a statement to the end of a statement list */
ASTNode* append_stmt(ASTNode *list, ASTNode *stmt) {
    if (!list) return stmt;
    ASTNode *curr = list;
    while (curr->next != NULL) {
        curr = curr->next;
    }
    curr->next = stmt;
    return list;
}
%}

/* Enable detailed syntax error messages in Bison 2.4.1 */
%error-verbose

/* Semantic Value Union */
%union {
    int ival;
    float fval;
    char *sval;
    DataType dtype;
    ASTNode *node;
}

/* Token Declarations with Display Names for Error Reporting */
%token T_INT    "int"
%token T_FLOAT  "float"
%token T_STRING "string"
%token T_BOOL   "bool"
%token T_IF     "if"
%token T_ELSE   "else"
%token T_DO     "do"
%token T_WHILE  "while"
%token T_PRINT  "print"

%token <sval> ID         "identifier"
%token <ival> INT_LIT    "integer literal"
%token <fval> FLOAT_LIT  "float literal"
%token <sval> STRING_LIT "string literal"
%token <ival> BOOL_LIT   "boolean literal"

%token EQ  "=="
%token NEQ "!="
%token LE  "<="
%token GE  ">="
%token AND "&&"
%token OR  "||"

/* Operator Precedence and Associativity (Lowest to Highest) */
%left OR
%left AND
%left EQ NEQ
%left '<' LE '>' GE
%left '+' '-'
%left '*' '/' '%'
%right '!' UMINUS

/* Resolve dangling-else ambiguity */
%nonassoc LOWER_THAN_ELSE
%nonassoc T_ELSE

/* Non-Terminal Types */
%type <node> program stmt_list stmt decl_stmt assign_stmt if_stmt while_stmt print_stmt block_stmt expr
%type <dtype> type_spec

%%

program:
    stmt_list {
        ast_root = create_block_node($1, 1);
    }
    ;

stmt_list:
    /* empty */ {
        $$ = NULL;
    }
    | stmt_list stmt {
        $$ = append_stmt($1, $2);
    }
    ;

stmt:
      decl_stmt   { $$ = $1; }
    | assign_stmt { $$ = $1; }
    | if_stmt     { $$ = $1; }
    | while_stmt  { $$ = $1; }
    | print_stmt  { $$ = $1; }
    | block_stmt  { $$ = $1; }
    | error ';'   {
        /* Panic-mode error recovery: skip to next semicolon */
        yyerrok;
        $$ = NULL;
    }
    | error '}'   {
        /* Panic-mode error recovery: skip to closing brace */
        yyerrok;
        $$ = NULL;
    }
    ;

type_spec:
      T_INT    { $$ = TYPE_INT; }
    | T_FLOAT  { $$ = TYPE_FLOAT; }
    | T_STRING { $$ = TYPE_STRING; }
    | T_BOOL   { $$ = TYPE_BOOL; }
    ;

/* Variable Declarations (supports single and comma-separated declarations) */
decl_stmt:
      type_spec ID ';' {
        $$ = create_decl_node($1, $2, NULL, yylineno);
    }
    | type_spec ID '=' expr ';' {
        $$ = create_decl_node($1, $2, $4, yylineno);
    }
    | type_spec ID ',' ID ';' {
        ASTNode *d1 = create_decl_node($1, $2, NULL, yylineno);
        ASTNode *d2 = create_decl_node($1, $4, NULL, yylineno);
        $$ = append_stmt(d1, d2);
    }
    ;

/* Assignment Statement */
assign_stmt:
    ID '=' expr ';' {
        $$ = create_assign_node($1, $3, yylineno);
    }
    ;

/* Control Flow: if and if-else */
if_stmt:
      T_IF '(' expr ')' stmt %prec LOWER_THAN_ELSE {
        $$ = create_if_node($3, $5, NULL, $3 ? $3->line_no : yylineno);
    }
    | T_IF '(' expr ')' stmt T_ELSE stmt {
        $$ = create_if_node($3, $5, $7, $3 ? $3->line_no : yylineno);
    }
    ;

/* Control Flow: do-while and while */
while_stmt:
      T_DO stmt T_WHILE '(' expr ')' ';' {
        $$ = create_dowhile_node($2, $5, yylineno);
    }
    | T_WHILE '(' expr ')' stmt {
        $$ = create_dowhile_node($5, $3, $3 ? $3->line_no : yylineno);
    }
    ;

/* Print Statement (supports both print(x); and print x;) */
print_stmt:
      T_PRINT '(' expr ')' ';' {
        $$ = create_print_node($3, yylineno);
    }
    | T_PRINT expr ';' {
        $$ = create_print_node($2, yylineno);
    }
    ;

/* Block Statement: { ... } */
block_stmt:
    '{' stmt_list '}' {
        $$ = create_block_node($2, yylineno);
    }
    ;

/* Expressions */
expr:
      expr '+' expr { $$ = create_binary_op_node("+",  $1, $3, $1->line_no); }
    | expr '-' expr { $$ = create_binary_op_node("-",  $1, $3, $1->line_no); }
    | expr '*' expr { $$ = create_binary_op_node("*",  $1, $3, $1->line_no); }
    | expr '/' expr { $$ = create_binary_op_node("/",  $1, $3, $1->line_no); }
    | expr '%' expr { $$ = create_binary_op_node("%",  $1, $3, $1->line_no); }
    | expr EQ  expr { $$ = create_binary_op_node("==", $1, $3, $1->line_no); }
    | expr NEQ expr { $$ = create_binary_op_node("!=", $1, $3, $1->line_no); }
    | expr '<' expr { $$ = create_binary_op_node("<",  $1, $3, $1->line_no); }
    | expr LE  expr { $$ = create_binary_op_node("<=", $1, $3, $1->line_no); }
    | expr '>' expr { $$ = create_binary_op_node(">",  $1, $3, $1->line_no); }
    | expr GE  expr { $$ = create_binary_op_node(">=", $1, $3, $1->line_no); }
    | expr AND expr { $$ = create_binary_op_node("&&", $1, $3, $1->line_no); }
    | expr OR  expr { $$ = create_binary_op_node("||", $1, $3, $1->line_no); }
    | '!' expr      { $$ = create_unary_op_node("!",   $2, $2->line_no); }
    | '-' expr %prec UMINUS { $$ = create_unary_op_node("-", $2, $2->line_no); }
    | '(' expr ')'  { $$ = $2; }
    | ID            { $$ = create_var_node($1, yylineno); }
    | INT_LIT       { $$ = create_int_node($1, yylineno); }
    | FLOAT_LIT     { $$ = create_float_node($1, yylineno); }
    | STRING_LIT    { $$ = create_string_node($1, yylineno); }
    | BOOL_LIT      { $$ = create_bool_node($1, yylineno); }
    ;

%%

/* Formats Bison's error string into:
   "Syntax Error at line N: expected X but found Y" */
void yyerror(const char *s) {
    syntax_errors++;
    const char *unexp = strstr(s, "unexpected ");
    const char *exp = strstr(s, "expecting ");

    if (unexp && exp) {
        char found[128] = {0};
        char expected[128] = {0};

        unexp += strlen("unexpected ");
        const char *comma = strstr(unexp, ", expecting ");
        if (comma) {
            size_t len = comma - unexp;
            if (len >= sizeof(found)) len = sizeof(found) - 1;
            strncpy(found, unexp, len);
        }
        exp += strlen("expecting ");
        strncpy(expected, exp, sizeof(expected) - 1);

        printf("Syntax Error at line %d: expected '%s' but found '%s'\n",
               yylineno, expected, found);
    } else if (unexp) {
        unexp += strlen("unexpected ");
        printf("Syntax Error at line %d: unexpected '%s' near '%s'\n",
               yylineno, unexp, yytext);
    } else {
        printf("Syntax Error at line %d: %s near '%s'\n",
               yylineno, s, yytext);
    }
}

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror("Error opening input file");
            return 1;
        }
    }

    init_symtab();
    yyparse();

    if (ast_root != NULL) {
        analyze_semantics(ast_root);
    }

    if (lexical_errors == 0 && syntax_errors == 0 && semantic_errors == 0) {
        printf("\n=== Compilation Successful (0 errors) ===\n\n");
        printf("--- Abstract Syntax Tree (AST) ---\n");
        print_ast(ast_root, 0);
        printf("\n--- Symbol Table ---\n");
        print_symtab();
    } else {
        printf("\n=== Compilation Failed (Lexical: %d, Syntax: %d, Semantic: %d) ===\n",
               lexical_errors, syntax_errors, semantic_errors);
    }

    if (yyin && yyin != stdin) {
        fclose(yyin);
    }
    return 0;
}
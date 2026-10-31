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

ASTNode* append_stmt(ASTNode *list, ASTNode *stmt) {
    if (!list) return stmt;
    ASTNode *curr = list;
    while (curr->next != NULL) curr = curr->next;
    curr->next = stmt;
    return list;
}
%}

%error-verbose

%union {
    int ival;
    float fval;
    char *sval;
    DataType dtype;
    ASTNode *node;
}

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

%token EQ "=="

/* Simplified Precedence */
%left EQ '<' '>'
%left '+' '-'
%left '*' '/'

%nonassoc LOWER_THAN_ELSE
%nonassoc T_ELSE

%type <node> program stmt_list stmt decl_stmt assign_stmt if_stmt while_stmt print_stmt block_stmt expr
%type <dtype> type_spec

%%

program:
    stmt_list { ast_root = create_block_node($1, 1); }
    ;

stmt_list:
    /* empty */ { $$ = NULL; }
    | stmt_list stmt { $$ = append_stmt($1, $2); }
    ;

stmt:
      decl_stmt   { $$ = $1; }
    | assign_stmt { $$ = $1; }
    | if_stmt     { $$ = $1; }
    | while_stmt  { $$ = $1; }
    | print_stmt  { $$ = $1; }
    | block_stmt  { $$ = $1; }
    ;

type_spec:
      T_INT    { $$ = TYPE_INT; }
    | T_FLOAT  { $$ = TYPE_FLOAT; }
    | T_STRING { $$ = TYPE_STRING; }
    | T_BOOL   { $$ = TYPE_BOOL; }
    ;

decl_stmt:
      type_spec ID ';'          { $$ = create_decl_node($1, $2, NULL, yylineno); }
    | type_spec ID '=' expr ';' { $$ = create_decl_node($1, $2, $4, yylineno); }
    ;

assign_stmt:
    ID '=' expr ';' { $$ = create_assign_node($1, $3, yylineno); }
    ;

if_stmt:
      T_IF '(' expr ')' stmt %prec LOWER_THAN_ELSE { $$ = create_if_node($3, $5, NULL, yylineno); }
    | T_IF '(' expr ')' stmt T_ELSE stmt           { $$ = create_if_node($3, $5, $7, yylineno); }
    ;

while_stmt:
    T_DO stmt T_WHILE '(' expr ')' ';' { $$ = create_dowhile_node($2, $5, yylineno); }
    ;

print_stmt:
    T_PRINT expr ';' { $$ = create_print_node($2, yylineno); }
    ;

block_stmt:
    '{' stmt_list '}' { $$ = create_block_node($2, yylineno); }
    ;

expr:
      expr '+' expr { $$ = create_binary_op_node("+",  $1, $3, $1->line_no); }
    | expr '-' expr { $$ = create_binary_op_node("-",  $1, $3, $1->line_no); }
    | expr '*' expr { $$ = create_binary_op_node("*",  $1, $3, $1->line_no); }
    | expr '/' expr { $$ = create_binary_op_node("/",  $1, $3, $1->line_no); }
    | expr '<' expr { $$ = create_binary_op_node("<",  $1, $3, $1->line_no); }
    | expr '>' expr { $$ = create_binary_op_node(">",  $1, $3, $1->line_no); }
    | expr EQ  expr { $$ = create_binary_op_node("==", $1, $3, $1->line_no); }
    | '(' expr ')'  { $$ = $2; }
    | ID            { $$ = create_var_node($1, yylineno); }
    | INT_LIT       { $$ = create_int_node($1, yylineno); }
    | FLOAT_LIT     { $$ = create_float_node($1, yylineno); }
    | STRING_LIT    { $$ = create_string_node($1, yylineno); }
    | BOOL_LIT      { $$ = create_bool_node($1, yylineno); }
    ;

%%

void yyerror(const char *s) {
    syntax_errors++;
    printf("Syntax Error at line %d: %s near '%s'\n", yylineno, s, yytext);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) { perror("Error opening file"); return 1; }
    }

    init_symtab();
    yyparse();

    if (ast_root != NULL) analyze_semantics(ast_root);

    if (lexical_errors == 0 && syntax_errors == 0 && semantic_errors == 0) {
        printf("\n=== Compilation Successful (0 errors) ===\n\n");
        print_ast(ast_root, 0);
        printf("\n--- Symbol Table ---\n");
        print_symtab();
    } else {
        printf("\n=== Compilation Failed (Lexical: %d, Syntax: %d, Semantic: %d) ===\n",
               lexical_errors, syntax_errors, semantic_errors);
    }

    if (yyin && yyin != stdin) fclose(yyin);
    return 0;
}
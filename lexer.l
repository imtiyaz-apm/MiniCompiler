%option yylineno
%option noyywrap

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"

int lexical_errors = 0;
%}

/* Regular Expression Definitions */
DIGIT       [0-9]
LETTER      [a-zA-Z_]
ID          {LETTER}({LETTER}|{DIGIT})*
INT_NUM     {DIGIT}+
FLOAT_NUM   {DIGIT}+"."{DIGIT}+

/* Malformed Numeric Patterns */
BAD_FLOAT   {DIGIT}+"."{DIGIT}+("."{DIGIT}*)+
BAD_NUM_ID  ({DIGIT}+|{FLOAT_NUM}){LETTER}({LETTER}|{DIGIT}|".")*

%%

 /* 1. Skip Single-Line and Multi-Line Block Comments */
"//".*                                  { /* Skip single-line comment */ }
"/*"([^*]|\*+[^*/])*\*+"/"              { /* Skip block comment */ }
"/*"([^*]|\*+[^*/])*                    {
    printf("Lexical Error at line %d: unterminated block comment\n", yylineno);
    lexical_errors++;
}

 /* 2. Skip Whitespace and Newlines (yylineno tracks lines automatically) */
[ \t\r\n]+                              { /* Ignore whitespace */ }

 /* 3. Keywords */
"int"                                   { return T_INT; }
"float"                                 { return T_FLOAT; }
"string"                                { return T_STRING; }
"bool"                                  { return T_BOOL; }
"if"                                    { return T_IF; }
"else"                                  { return T_ELSE; }
"do"                                    { return T_DO; }
"while"                                 { return T_WHILE; }
"print"                                 { return T_PRINT; }

 /* 4. Boolean Literals */
"true"                                  { yylval.ival = 1; return BOOL_LIT; }
"false"                                 { yylval.ival = 0; return BOOL_LIT; }

 /* 5. Catch Malformed Numeric Literals (Must appear before valid numbers) */
{BAD_FLOAT} {
    printf("Lexical Error at line %d: malformed numeric literal '%s'\n", yylineno, yytext);
    lexical_errors++;
    yylval.fval = 0.0;
    return FLOAT_LIT;
}

{BAD_NUM_ID} {
    printf("Lexical Error at line %d: invalid numeric literal '%s'\n", yylineno, yytext);
    lexical_errors++;
    yylval.ival = 0;
    return INT_LIT;
}

 /* 6. Valid Numeric and Identifier Tokens */
{FLOAT_NUM}                             { yylval.fval = atof(yytext); return FLOAT_LIT; }
{INT_NUM}                               { yylval.ival = atoi(yytext); return INT_LIT; }
{ID}                                    { yylval.sval = strdup(yytext); return ID; }

 /* 7. String Literals and Unterminated Strings */
\"([^\"\\\n]|\\.)*\"                    { yylval.sval = strdup(yytext); return STRING_LIT; }
\"([^\"\\\n]|\\.)*                      {
    printf("Lexical Error at line %d: unterminated string literal\n", yylineno);
    lexical_errors++;
    yylval.sval = strdup("\"\"");
    return STRING_LIT;
}

 /* 8. Multi-Character Operators */
"=="                                    { return EQ; }


 /* 9. Single-Character Operators and Punctuation */
"+"|"-"|"*"|"/"|"%"|"="|"<"|">"|"!"|"("|")"|"{"|"}"|";"|"," {
    return yytext[0];
}

 /* 10. Illegal / Unrecognized Characters */
. {
    printf("Lexical Error at line %d: unrecognized character '%s'\n", yylineno, yytext);
    lexical_errors++;
}

%%
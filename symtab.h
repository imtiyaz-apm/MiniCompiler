#ifndef SYMTAB_H
#define SYMTAB_H

#include "ast.h"

/* Symbol Table Entry (Part C) */
typedef struct Symbol {
    char name[64];              /* a) Identifier name */
    DataType type;              /* b) Data type */
    int scope_level;            /* c) Scope level */
    int decl_line;              /* d) Line number of declaration */
    int is_init;                /* e) Initialization status (1 = yes, 0 = no) */
    int is_active;              /* 1 if scope is currently active, 0 if exited */
    struct Symbol *next;
} Symbol;

/* Symbol Table Management */
void init_symtab();
void enter_scope();
void exit_scope();
int insert_symbol(char *name, DataType type, int line, int is_init);
Symbol* lookup_symbol(char *name);
Symbol* lookup_current_scope(char *name);
void print_symtab();

/* Semantic Analysis (Part D) */
void analyze_semantics(ASTNode *root);

#endif
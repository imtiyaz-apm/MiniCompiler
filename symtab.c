#include "symtab.h"

static Symbol *symbol_table = NULL;
static int current_scope = -1; /* Starts at -1 so root BlockNode enters scope 0 (global) */
int semantic_errors = 0;

/* Initialize or reset the symbol table */
void init_symtab() {
    symbol_table = NULL;
    current_scope = -1;
    semantic_errors = 0;
}

/* Enter a new nested block scope */
void enter_scope() {
    current_scope++;
}

/* Exit the current scope (marks symbols inactive rather than deleting them
   so we can still print the full symbol table and detect scope violations) */
void exit_scope() {
    Symbol *curr = symbol_table;
    while (curr != NULL) {
        if (curr->scope_level == current_scope && curr->is_active) {
            curr->is_active = 0;
        }
        curr = curr->next;
    }
    current_scope--;
}

/* Search only the current active scope (for duplicate declaration checks) */
Symbol* lookup_current_scope(char *name) {
    Symbol *curr = symbol_table;
    while (curr != NULL) {
        if (curr->is_active && curr->scope_level == current_scope &&
            strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

/* Search all active scopes from innermost to outermost */
Symbol* lookup_symbol(char *name) {
    Symbol *curr = symbol_table;
    Symbol *best_match = NULL;

    while (curr != NULL) {
        if (curr->is_active && strcmp(curr->name, name) == 0) {
            if (best_match == NULL || curr->scope_level > best_match->scope_level) {
                best_match = curr;
            }
        }
        curr = curr->next;
    }
    return best_match;
}

/* Search inactive (exited) scopes to detect scope violations (Bonus e) */
static Symbol* lookup_out_of_scope(char *name) {
    Symbol *curr = symbol_table;
    while (curr != NULL) {
        if (!curr->is_active && strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

/* Insert a new symbol into the current scope */
int insert_symbol(char *name, DataType type, int line, int is_init) {
    Symbol *existing = lookup_current_scope(name);
    if (existing != NULL) {
        printf("Semantic Error at line %d: duplicate declaration of variable '%s' (previously declared at line %d)\n",
               line, name, existing->decl_line);
        semantic_errors++;
        return 0;
    }

    Symbol *new_sym = (Symbol*)calloc(1, sizeof(Symbol));
    if (!new_sym) {
        fprintf(stderr, "Memory allocation failed for Symbol\n");
        exit(1);
    }

    strncpy(new_sym->name, name, sizeof(new_sym->name) - 1);
    new_sym->type = type;
    new_sym->scope_level = current_scope;
    new_sym->decl_line = line;
    new_sym->is_init = is_init;
    new_sym->is_active = 1;
    new_sym->next = symbol_table;
    symbol_table = new_sym;

    return 1;
}

/* Print all recorded entries in the Symbol Table */
void print_symtab() {
    printf("%-15s %-10s %-10s %-10s %-12s\n",
           "Identifier", "Type", "Scope", "Line", "Initialized");
    printf("---------------------------------------------------------------\n");

    /* Helper recursion or simple loop to print in declaration order */
    int count = 0;
    Symbol *curr = symbol_table;
    while (curr) {
        count++;
        curr = curr->next;
    }

    if (count == 0) {
        printf("(No symbols declared)\n");
        return;
    }

    Symbol **arr = (Symbol**)malloc(count * sizeof(Symbol*));
    curr = symbol_table;
    for (int i = count - 1; i >= 0; i--) {
        arr[i] = curr;
        curr = curr->next;
    }

    for (int i = 0; i < count; i++) {
        printf("%-15s %-10s %-10d %-10d %-12s\n",
               arr[i]->name,
               type_to_str(arr[i]->type),
               arr[i]->scope_level,
               arr[i]->decl_line,
               arr[i]->is_init ? "Yes" : "No");
    }
    free(arr);
}

/* Forward declaration for expression evaluation */
static DataType evaluate_expr(ASTNode *expr);

/* Evaluate expression types and check for semantic errors */
static DataType evaluate_expr(ASTNode *expr) {
    if (!expr) return TYPE_VOID;

    switch (expr->node_type) {
        case NODE_INT_LIT:
            return TYPE_INT;
        case NODE_FLOAT_LIT:
            return TYPE_FLOAT;
        case NODE_STRING_LIT:
            return TYPE_STRING;
        case NODE_BOOL_LIT:
            return TYPE_BOOL;

        case NODE_VAR: {
            Symbol *sym = lookup_symbol(expr->name);
            if (!sym) {
                Symbol *out_sym = lookup_out_of_scope(expr->name);
                if (out_sym) {
                    /* Bonus e: Scope violation */
                    printf("Semantic Error at line %d: scope violation — variable '%s' (declared at line %d) is out of scope\n",
                           expr->line_no, expr->name, out_sym->decl_line);
                } else {
                    /* Part D(a): Undeclared variable usage */
                    printf("Semantic Error at line %d: variable '%s' used before declaration\n",
                           expr->line_no, expr->name);
                }
                semantic_errors++;
                expr->data_type = TYPE_ERROR;
                return TYPE_ERROR;
            }

            /* Bonus f: Uninitialized variable use */
            if (!sym->is_init) {
                printf("Semantic Error at line %d: variable '%s' used before initialization\n",
                       expr->line_no, expr->name);
                semantic_errors++;
            }

            expr->data_type = sym->type;
            return sym->type;
        }

        case NODE_UNARY_OP: {
            DataType op_type = evaluate_expr(expr->left);
            if (op_type == TYPE_ERROR) return TYPE_ERROR;

            if (strcmp(expr->op, "!") == 0) {
                if (op_type != TYPE_BOOL) {
                    printf("Semantic Error at line %d: type mismatch — logical '!' requires 'bool' operand, found '%s'\n",
                           expr->line_no, type_to_str(op_type));
                    semantic_errors++;
                    return TYPE_ERROR;
                }
                expr->data_type = TYPE_BOOL;
                return TYPE_BOOL;
            } else if (strcmp(expr->op, "-") == 0) {
                if (op_type != TYPE_INT && op_type != TYPE_FLOAT) {
                    printf("Semantic Error at line %d: type mismatch — unary '-' requires numeric operand, found '%s'\n",
                           expr->line_no, type_to_str(op_type));
                    semantic_errors++;
                    return TYPE_ERROR;
                }
                expr->data_type = op_type;
                return op_type;
            }
            return TYPE_ERROR;
        }

        case NODE_BINARY_OP: {
            DataType left_t = evaluate_expr(expr->left);
            DataType right_t = evaluate_expr(expr->right);

            if (left_t == TYPE_ERROR || right_t == TYPE_ERROR) {
                expr->data_type = TYPE_ERROR;
                return TYPE_ERROR;
            }

            char *op = expr->op;

            /* Arithmetic Operators: +, -, *, / */
            if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
                strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
                if (left_t != right_t || (left_t != TYPE_INT && left_t != TYPE_FLOAT)) {
                    printf("Semantic Error at line %d: type mismatch in expression — cannot apply '%s' to '%s' and '%s'\n",
                           expr->line_no, op, type_to_str(left_t), type_to_str(right_t));
                    semantic_errors++;
                    expr->data_type = TYPE_ERROR;
                    return TYPE_ERROR;
                }
                expr->data_type = left_t;
                return left_t;
            }

            /* Modulo Operator: % (strictly int) */
            if (strcmp(op, "%") == 0) {
                if (left_t != TYPE_INT || right_t != TYPE_INT) {
                    printf("Semantic Error at line %d: type mismatch in expression — modulo '%%' requires 'int' operands, found '%s' and '%s'\n",
                           expr->line_no, type_to_str(left_t), type_to_str(right_t));
                    semantic_errors++;
                    expr->data_type = TYPE_ERROR;
                    return TYPE_ERROR;
                }
                expr->data_type = TYPE_INT;
                return TYPE_INT;
            }

            /* Relational Operators: <, <=, >, >= */
            if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
                strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
                if (left_t != right_t || (left_t != TYPE_INT && left_t != TYPE_FLOAT)) {
                    printf("Semantic Error at line %d: type mismatch in expression — cannot compare '%s' and '%s' with '%s'\n",
                           expr->line_no, type_to_str(left_t), type_to_str(right_t), op);
                    semantic_errors++;
                    expr->data_type = TYPE_ERROR;
                    return TYPE_ERROR;
                }
                expr->data_type = TYPE_BOOL;
                return TYPE_BOOL;
            }

            /* Equality Operators: ==, != */
            if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
                if (left_t != right_t) {
                    printf("Semantic Error at line %d: type mismatch in expression — cannot compare '%s' and '%s' with '%s'\n",
                           expr->line_no, type_to_str(left_t), type_to_str(right_t), op);
                    semantic_errors++;
                    expr->data_type = TYPE_ERROR;
                    return TYPE_ERROR;
                }
                expr->data_type = TYPE_BOOL;
                return TYPE_BOOL;
            }

            /* Logical Operators: &&, || */
            if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
                if (left_t != TYPE_BOOL || right_t != TYPE_BOOL) {
                    printf("Semantic Error at line %d: type mismatch in expression — logical '%s' requires 'bool' operands, found '%s' and '%s'\n",
                           expr->line_no, op, type_to_str(left_t), type_to_str(right_t));
                    semantic_errors++;
                    expr->data_type = TYPE_ERROR;
                    return TYPE_ERROR;
                }
                expr->data_type = TYPE_BOOL;
                return TYPE_BOOL;
            }

            return TYPE_ERROR;
        }

        default:
            return TYPE_VOID;
    }
}

/* Traverse AST statements recursively */
void analyze_semantics(ASTNode *node) {
    while (node != NULL) {
        switch (node->node_type) {
            case NODE_BLOCK:
                enter_scope();
                analyze_semantics(node->body);
                exit_scope();
                break;

            case NODE_DECL: {
                int is_initialized = 0;
                if (node->left != NULL) {
                    DataType init_type = evaluate_expr(node->left);
                    if (init_type != TYPE_ERROR && init_type != node->data_type) {
                        /* Bonus d: Assignment type mismatch during initialization */
                        printf("Semantic Error at line %d: type mismatch — cannot assign '%s' to variable of type '%s'\n",
                               node->line_no, type_to_str(init_type), type_to_str(node->data_type));
                        semantic_errors++;
                    } else if (init_type != TYPE_ERROR) {
                        is_initialized = 1;
                    }
                }
                insert_symbol(node->name, node->data_type, node->line_no, is_initialized);
                break;
            }

            case NODE_ASSIGN: {
                Symbol *sym = lookup_symbol(node->name);
                DataType rhs_type = evaluate_expr(node->left);

                if (!sym) {
                    Symbol *out_sym = lookup_out_of_scope(node->name);
                    if (out_sym) {
                        /* Bonus e: Scope violation */
                        printf("Semantic Error at line %d: scope violation — variable '%s' (declared at line %d) is out of scope\n",
                               node->line_no, node->name, out_sym->decl_line);
                    } else {
                        /* Part D(a): Undeclared variable usage */
                        printf("Semantic Error at line %d: variable '%s' used before declaration\n",
                               node->line_no, node->name);
                    }
                    semantic_errors++;
                } else if (rhs_type != TYPE_ERROR) {
                    /* Bonus d: Assignment type mismatch */
                    if (sym->type != rhs_type) {
                        printf("Semantic Error at line %d: type mismatch — cannot assign '%s' to variable of type '%s'\n",
                               node->line_no, type_to_str(rhs_type), type_to_str(sym->type));
                        semantic_errors++;
                    } else {
                        sym->is_init = 1;
                    }
                }
                break;
            }

            case NODE_IF: {
                DataType cond_type = evaluate_expr(node->cond);
                if (cond_type != TYPE_ERROR && cond_type != TYPE_BOOL) {
                    printf("Semantic Error at line %d: type mismatch — 'if' condition must be of type 'bool', found '%s'\n",
                           node->line_no, type_to_str(cond_type));
                    semantic_errors++;
                }
                analyze_semantics(node->body);
                if (node->else_body) {
                    analyze_semantics(node->else_body);
                }
                break;
            }

            case NODE_DO_WHILE: {
                analyze_semantics(node->body);
                DataType cond_type = evaluate_expr(node->cond);
                if (cond_type != TYPE_ERROR && cond_type != TYPE_BOOL) {
                    printf("Semantic Error at line %d: type mismatch — 'while' condition must be of type 'bool', found '%s'\n",
                           node->line_no, type_to_str(cond_type));
                    semantic_errors++;
                }
                break;
            }

            case NODE_PRINT:
                evaluate_expr(node->left);
                break;

            default:
                break;
        }
        node = node->next;
    }
}
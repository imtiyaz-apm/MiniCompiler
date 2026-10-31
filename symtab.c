#include "symtab.h"

static Symbol *symbol_table = NULL;
static int current_scope = -1;
int semantic_errors = 0;

void init_symtab() {
    symbol_table = NULL;
    current_scope = -1;
    semantic_errors = 0;
}

void enter_scope() { current_scope++; }

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

Symbol* lookup_current_scope(char *name) {
    Symbol *curr = symbol_table;
    while (curr != NULL) {
        if (curr->is_active && curr->scope_level == current_scope && strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

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

int insert_symbol(char *name, DataType type, int line, int is_init) {
    Symbol *existing = lookup_current_scope(name);
    if (existing != NULL) {
        printf("Semantic Error at line %d: duplicate declaration of variable '%s'\n", line, name);
        semantic_errors++;
        return 0;
    }

    Symbol *new_sym = (Symbol*)calloc(1, sizeof(Symbol));
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

void print_symtab() {
    printf("%-15s %-10s %-10s %-10s %-12s\n", "Identifier", "Type", "Scope", "Line", "Initialized");
    printf("---------------------------------------------------------------\n");
    Symbol *curr = symbol_table;
    while (curr) {
        printf("%-15s %-10s %-10d %-10d %-12s\n",
               curr->name, type_to_str(curr->type), curr->scope_level, curr->decl_line, curr->is_init ? "Yes" : "No");
        curr = curr->next;
    }
}

static DataType evaluate_expr(ASTNode *expr) {
    if (!expr) return TYPE_VOID;

    switch (expr->node_type) {
        case NODE_INT_LIT:    return TYPE_INT;
        case NODE_FLOAT_LIT:  return TYPE_FLOAT;
        case NODE_STRING_LIT: return TYPE_STRING;
        case NODE_BOOL_LIT:   return TYPE_BOOL;

        case NODE_VAR: {
            Symbol *sym = lookup_symbol(expr->name);
            if (!sym) {
                printf("Semantic Error at line %d: variable '%s' used before declaration\n", expr->line_no, expr->name);
                semantic_errors++;
                return TYPE_ERROR;
            }
            /* Check if variable was used before initialization */
            if (!sym->is_init) {
                printf("Semantic Error at line %d: variable '%s' used before initialization\n", expr->line_no, expr->name);
                semantic_errors++;
            }
            return sym->type;
        }

        case NODE_BINARY_OP: {
            DataType left_t = evaluate_expr(expr->left);
            DataType right_t = evaluate_expr(expr->right);

            if (left_t == TYPE_ERROR || right_t == TYPE_ERROR) return TYPE_ERROR;

            if (strcmp(expr->op, "+") == 0 || strcmp(expr->op, "-") == 0 ||
                strcmp(expr->op, "*") == 0 || strcmp(expr->op, "/") == 0) {
                if (left_t != right_t || (left_t != TYPE_INT && left_t != TYPE_FLOAT)) {
                    printf("Semantic Error at line %d: type mismatch in arithmetic expression\n", expr->line_no);
                    semantic_errors++;
                    return TYPE_ERROR;
                }
                return left_t;
            }

            if (strcmp(expr->op, "<") == 0 || strcmp(expr->op, ">") == 0 || strcmp(expr->op, "==") == 0) {
                if (left_t != right_t) {
                    printf("Semantic Error at line %d: type mismatch in comparison\n", expr->line_no);
                    semantic_errors++;
                    return TYPE_ERROR;
                }
                return TYPE_BOOL;
            }
            return TYPE_ERROR;
        }
        default: return TYPE_VOID;
    }
}

void analyze_semantics(ASTNode *node) {
    while (node != NULL) {
        switch (node->node_type) {
            case NODE_BLOCK:
                enter_scope();
                analyze_semantics(node->body);
                exit_scope();
                break;

            case NODE_DECL: {
                int is_init = 0;
                if (node->left != NULL) {
                    DataType init_type = evaluate_expr(node->left);
                    if (init_type != TYPE_ERROR && init_type != node->data_type) {
                        printf("Semantic Error at line %d: type mismatch in assignment/initialization\n", node->line_no);
                        semantic_errors++;
                    } else if (init_type != TYPE_ERROR) {
                        is_init = 1;
                    }
                }
                insert_symbol(node->name, node->data_type, node->line_no, is_init);
                break;
            }

            case NODE_ASSIGN: {
                Symbol *sym = lookup_symbol(node->name);
                DataType rhs_type = evaluate_expr(node->left);
                if (!sym) {
                    printf("Semantic Error at line %d: variable '%s' used before declaration\n", node->line_no, node->name);
                    semantic_errors++;
                } else if (rhs_type != TYPE_ERROR) {
                    if (sym->type != rhs_type) {
                        printf("Semantic Error at line %d: type mismatch in assignment\n", node->line_no);
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
                    printf("Semantic Error at line %d: 'if' condition must be 'bool'\n", node->line_no);
                    semantic_errors++;
                }
                analyze_semantics(node->body);
                if (node->else_body) analyze_semantics(node->else_body);
                break;
            }

            case NODE_DO_WHILE: {
                analyze_semantics(node->body);
                DataType cond_type = evaluate_expr(node->cond);
                if (cond_type != TYPE_ERROR && cond_type != TYPE_BOOL) {
                    printf("Semantic Error at line %d: 'while' condition must be 'bool'\n", node->line_no);
                    semantic_errors++;
                }
                break;
            }

            case NODE_PRINT:
                evaluate_expr(node->left);
                break;

            default: break;
        }
        node = node->next;
    }
}
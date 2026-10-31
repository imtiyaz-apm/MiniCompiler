#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Supported Data Types */
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_VOID,
    TYPE_ERROR
} DataType;

/* Simplified AST Node Categories (No Unary Ops) */
typedef enum {
    NODE_BLOCK,
    NODE_DECL,
    NODE_ASSIGN,
    NODE_BINARY_OP,
    NODE_IF,
    NODE_DO_WHILE,
    NODE_PRINT,
    NODE_VAR,
    NODE_INT_LIT,
    NODE_FLOAT_LIT,
    NODE_STRING_LIT,
    NODE_BOOL_LIT
} NodeType;

/* Unified AST Node Structure */
typedef struct ASTNode {
    NodeType node_type;
    DataType data_type;
    int line_no;

    char *name;
    char *op;

    int int_val;
    float float_val;
    char *str_val;
    int bool_val;

    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *cond;
    struct ASTNode *body;
    struct ASTNode *else_body;
    struct ASTNode *next;
} ASTNode;

/* AST Constructor Prototypes */
ASTNode* create_node(NodeType type, int line);
ASTNode* create_decl_node(DataType type, char *name, ASTNode *init_expr, int line);
ASTNode* create_assign_node(char *name, ASTNode *expr, int line);
ASTNode* create_binary_op_node(char *op, ASTNode *left, ASTNode *right, int line);
ASTNode* create_if_node(ASTNode *cond, ASTNode *then_branch, ASTNode *else_branch, int line);
ASTNode* create_dowhile_node(ASTNode *body, ASTNode *cond, int line);
ASTNode* create_print_node(ASTNode *expr, int line);
ASTNode* create_block_node(ASTNode *stmt_list, int line);
ASTNode* create_var_node(char *name, int line);
ASTNode* create_int_node(int val, int line);
ASTNode* create_float_node(float val, int line);
ASTNode* create_string_node(char *val, int line);
ASTNode* create_bool_node(int val, int line);

void print_ast(ASTNode *node, int indent);
const char* type_to_str(DataType type);

#endif
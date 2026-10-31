#include "ast.h"

/* Base helper to allocate and initialize a clean ASTNode */
ASTNode* create_node(NodeType type, int line) {
    ASTNode *node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed for ASTNode\n");
        exit(1);
    }
    node->node_type = type;
    node->data_type = TYPE_VOID;
    node->line_no = line;
    return node;
}

/* Variable Declaration Node: e.g., int x = 5; or int x; */
ASTNode* create_decl_node(DataType type, char *name, ASTNode *init_expr, int line) {
    ASTNode *node = create_node(NODE_DECL, line);
    node->data_type = type;
    node->name = strdup(name);
    node->left = init_expr; /* NULL if uninitialized */
    return node;
}

/* Assignment Node: e.g., x = a + b; */
ASTNode* create_assign_node(char *name, ASTNode *expr, int line) {
    ASTNode *node = create_node(NODE_ASSIGN, line);
    node->name = strdup(name);
    node->left = expr;
    return node;
}

/* Binary Operation Node: e.g., a + b, x == y, p && q */
ASTNode* create_binary_op_node(char *op, ASTNode *left, ASTNode *right, int line) {
    ASTNode *node = create_node(NODE_BINARY_OP, line);
    node->op = strdup(op);
    node->left = left;
    node->right = right;
    return node;
}

/* Unary Operation Node: e.g., !flag, -x */
ASTNode* create_unary_op_node(char *op, ASTNode *operand, int line) {
    ASTNode *node = create_node(NODE_UNARY_OP, line);
    node->op = strdup(op);
    node->left = operand;
    return node;
}

/* If-Else Node */
ASTNode* create_if_node(ASTNode *cond, ASTNode *then_branch, ASTNode *else_branch, int line) {
    ASTNode *node = create_node(NODE_IF, line);
    node->cond = cond;
    node->body = then_branch;
    node->else_body = else_branch; /* NULL if no else branch */
    return node;
}

/* Do-While Node */
ASTNode* create_dowhile_node(ASTNode *body, ASTNode *cond, int line) {
    ASTNode *node = create_node(NODE_DO_WHILE, line);
    node->body = body;
    node->cond = cond;
    return node;
}

/* Print Statement Node */
ASTNode* create_print_node(ASTNode *expr, int line) {
    ASTNode *node = create_node(NODE_PRINT, line);
    node->left = expr;
    return node;
}

/* Block Node: { stmt_list } */
ASTNode* create_block_node(ASTNode *stmt_list, int line) {
    ASTNode *node = create_node(NODE_BLOCK, line);
    node->body = stmt_list;
    return node;
}

/* Variable Usage (Identifier) Node */
ASTNode* create_var_node(char *name, int line) {
    ASTNode *node = create_node(NODE_VAR, line);
    node->name = strdup(name);
    return node;
}

/* Literal Nodes */
ASTNode* create_int_node(int val, int line) {
    ASTNode *node = create_node(NODE_INT_LIT, line);
    node->data_type = TYPE_INT;
    node->int_val = val;
    return node;
}

ASTNode* create_float_node(float val, int line) {
    ASTNode *node = create_node(NODE_FLOAT_LIT, line);
    node->data_type = TYPE_FLOAT;
    node->float_val = val;
    return node;
}

ASTNode* create_string_node(char *val, int line) {
    ASTNode *node = create_node(NODE_STRING_LIT, line);
    node->data_type = TYPE_STRING;
    node->str_val = strdup(val);
    return node;
}

ASTNode* create_bool_node(int val, int line) {
    ASTNode *node = create_node(NODE_BOOL_LIT, line);
    node->data_type = TYPE_BOOL;
    node->bool_val = val;
    return node;
}

/* Helper to convert DataType enum to string for error messages */
const char* type_to_str(DataType type) {
    switch (type) {
        case TYPE_INT:    return "int";
        case TYPE_FLOAT:  return "float";
        case TYPE_STRING: return "string";
        case TYPE_BOOL:   return "bool";
        case TYPE_VOID:   return "void";
        default:          return "error";
    }
}

/* Helper to print the AST structure visually */
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void print_ast(ASTNode *node, int indent) {
    while (node != NULL) {
        print_indent(indent);
        switch (node->node_type) {
            case NODE_BLOCK:
                printf("BlockNode (line %d)\n", node->line_no);
                print_ast(node->body, indent + 1);
                break;
            case NODE_DECL:
                printf("DeclNode [type: %s, id: %s] (line %d)\n",
                       type_to_str(node->data_type), node->name, node->line_no);
                if (node->left) {
                    print_ast(node->left, indent + 1);
                }
                break;
            case NODE_ASSIGN:
                printf("AssignNode [id: %s] (line %d)\n", node->name, node->line_no);
                print_ast(node->left, indent + 1);
                break;
            case NODE_BINARY_OP:
                printf("BinaryOpNode [op: %s] (line %d)\n", node->op, node->line_no);
                print_ast(node->left, indent + 1);
                print_ast(node->right, indent + 1);
                break;
            case NODE_UNARY_OP:
                printf("UnaryOpNode [op: %s] (line %d)\n", node->op, node->line_no);
                print_ast(node->left, indent + 1);
                break;
            case NODE_IF:
                printf("IfNode (line %d)\n", node->line_no);
                print_indent(indent + 1);
                printf("Condition:\n");
                print_ast(node->cond, indent + 2);
                print_indent(indent + 1);
                printf("Then:\n");
                print_ast(node->body, indent + 2);
                if (node->else_body) {
                    print_indent(indent + 1);
                    printf("Else:\n");
                    print_ast(node->else_body, indent + 2);
                }
                break;
            case NODE_DO_WHILE:
                printf("WhileNode [do-while] (line %d)\n", node->line_no);
                print_indent(indent + 1);
                printf("Body:\n");
                print_ast(node->body, indent + 2);
                print_indent(indent + 1);
                printf("Condition:\n");
                print_ast(node->cond, indent + 2);
                break;
            case NODE_PRINT:
                printf("PrintNode (line %d)\n", node->line_no);
                print_ast(node->left, indent + 1);
                break;
            case NODE_VAR:
                printf("VarNode [id: %s] (line %d)\n", node->name, node->line_no);
                break;
            case NODE_INT_LIT:
                printf("IntLiteral [%d] (line %d)\n", node->int_val, node->line_no);
                break;
            case NODE_FLOAT_LIT:
                printf("FloatLiteral [%.2f] (line %d)\n", node->float_val, node->line_no);
                break;
            case NODE_STRING_LIT:
                printf("StringLiteral [%s] (line %d)\n", node->str_val, node->line_no);
                break;
            case NODE_BOOL_LIT:
                printf("BoolLiteral [%s] (line %d)\n", node->bool_val ? "true" : "false", node->line_no);
                break;
        }
        node = node->next;
    }
}
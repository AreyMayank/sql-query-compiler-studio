#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    NODE_SELECT_STMT,
    NODE_COLUMN_LIST,
    NODE_COLUMN,
    NODE_STAR,
    NODE_TABLE_NAME,
    NODE_WHERE_CLAUSE,
    NODE_BINARY_EXPR,
    NODE_UNARY_EXPR,
    NODE_IDENTIFIER,
    NODE_INT_LITERAL,
    NODE_FLOAT_LITERAL,
    NODE_STRING_LITERAL,
    NODE_BOOL_LITERAL,
    NODE_ORDER_BY_CLAUSE,
    NODE_ORDER_ITEM,
    NODE_LIMIT_CLAUSE
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char *string_val;
    int int_val;
    double float_val;
    char *op_val;
    struct ASTNode **children;
    int num_children;
    int capacity;
    int line;
    int column;
} ASTNode;

ASTNode* create_ast_node(ASTNodeType type);
void add_ast_child(ASTNode *parent, ASTNode *child);
void free_ast(ASTNode *node);
const char* ast_node_type_to_string(ASTNodeType type);
void save_ast_to_json(ASTNode *node, const char *filename);

#endif // AST_H

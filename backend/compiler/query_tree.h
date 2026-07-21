#ifndef QUERY_TREE_H
#define QUERY_TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

typedef enum {
    QT_SCAN,
    QT_SELECTION,
    QT_PROJECTION,
    QT_SORT,
    QT_LIMIT
} QueryTreeNodeType;

typedef struct QueryTreeNode {
    QueryTreeNodeType type;
    char *value;
    struct QueryTreeNode *child;
} QueryTreeNode;

QueryTreeNode* create_query_tree_node(QueryTreeNodeType type, const char *value);
QueryTreeNode* build_query_tree_from_ast(ASTNode *ast_root);
void free_query_tree(QueryTreeNode *node);
const char* query_tree_node_type_to_string(QueryTreeNodeType type);
void save_query_tree_to_json(QueryTreeNode *root, const char *filename);
char* ast_expr_to_string(ASTNode *expr);

#endif // QUERY_TREE_H

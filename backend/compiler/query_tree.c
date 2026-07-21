#include "query_tree.h"

QueryTreeNode* create_query_tree_node(QueryTreeNodeType type, const char *value) {
    QueryTreeNode *node = (QueryTreeNode*)malloc(sizeof(QueryTreeNode));
    if (!node) {
        perror("Out of memory");
        exit(EXIT_FAILURE);
    }
    node->type = type;
    node->value = value ? strdup(value) : NULL;
    node->child = NULL;
    return node;
}

char* ast_expr_to_string(ASTNode *expr) {
    if (!expr) return strdup("");

    if (expr->type == NODE_INT_LITERAL) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", expr->int_val);
        return strdup(buf);
    }
    if (expr->type == NODE_FLOAT_LITERAL) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", expr->float_val);
        return strdup(buf);
    }
    if (expr->type == NODE_STRING_LITERAL) {
        char buf[256];
        snprintf(buf, sizeof(buf), "'%s'", expr->string_val);
        return strdup(buf);
    }
    if (expr->type == NODE_BOOL_LITERAL) {
        return strdup(expr->int_val ? "TRUE" : "FALSE");
    }
    if (expr->type == NODE_IDENTIFIER) {
        return strdup(expr->string_val);
    }
    if (expr->type == NODE_STAR) {
        return strdup("*");
    }
    if (expr->type == NODE_COLUMN) {
        return strdup(expr->string_val);
    }

    if (expr->type == NODE_BINARY_EXPR) {
        char *left = ast_expr_to_string(expr->children[0]);
        char *right = ast_expr_to_string(expr->children[1]);
        int len = strlen(left) + strlen(right) + strlen(expr->op_val) + 8;
        char *res = (char*)malloc(len);
        snprintf(res, len, "(%s %s %s)", left, expr->op_val, right);
        free(left);
        free(right);
        return res;
    }

    if (expr->type == NODE_UNARY_EXPR) {
        char *child = ast_expr_to_string(expr->children[0]);
        int len = strlen(child) + strlen(expr->op_val) + 8;
        char *res = (char*)malloc(len);
        snprintf(res, len, "%s (%s)", expr->op_val, child);
        free(child);
        return res;
    }

    if (expr->type == NODE_COLUMN_LIST) {
        int capacity = 256;
        char *res = (char*)malloc(capacity);
        res[0] = '\0';
        for (int i = 0; i < expr->num_children; i++) {
            char *col_str = ast_expr_to_string(expr->children[i]);
            if (i > 0) {
                if (strlen(res) + strlen(col_str) + 3 >= capacity) {
                    capacity *= 2;
                    res = (char*)realloc(res, capacity);
                }
                strcat(res, ", ");
            }
            strcat(res, col_str);
            free(col_str);
        }
        return res;
    }

    return strdup("");
}

QueryTreeNode* build_query_tree_from_ast(ASTNode *ast_root) {
    if (!ast_root || ast_root->type != NODE_SELECT_STMT) return NULL;

    // Retrieve components from AST
    ASTNode *select_list = ast_root->children[0];
    ASTNode *table_node = ast_root->children[1];
    ASTNode *where_clause = ast_root->children[2];
    ASTNode *order_clause = ast_root->children[3];
    ASTNode *limit_clause = ast_root->children[4];

    // Relational algebra builds bottom up: Scan -> Selection -> Projection -> Sort -> Limit
    // Scan Node
    QueryTreeNode *current = create_query_tree_node(QT_SCAN, table_node->string_val);

    // Selection Node
    if (where_clause && where_clause->num_children > 0) {
        char *cond_str = ast_expr_to_string(where_clause->children[0]);
        QueryTreeNode *sel = create_query_tree_node(QT_SELECTION, cond_str);
        free(cond_str);
        sel->child = current;
        current = sel;
    }

    // Projection Node
    char *proj_str = ast_expr_to_string(select_list);
    QueryTreeNode *proj = create_query_tree_node(QT_PROJECTION, proj_str);
    free(proj_str);
    proj->child = current;
    current = proj;

    // Sort Node
    if (order_clause && order_clause->num_children > 0) {
        ASTNode *order_item = order_clause->children[0];
        int len = strlen(order_item->string_val) + strlen(order_item->op_val) + 4;
        char *sort_val = (char*)malloc(len);
        snprintf(sort_val, len, "%s %s", order_item->string_val, order_item->op_val);
        QueryTreeNode *sort = create_query_tree_node(QT_SORT, sort_val);
        free(sort_val);
        sort->child = current;
        current = sort;
    }

    // Limit Node
    if (limit_clause) {
        char limit_str[32];
        snprintf(limit_str, sizeof(limit_str), "%d", limit_clause->int_val);
        QueryTreeNode *lim = create_query_tree_node(QT_LIMIT, limit_str);
        lim->child = current;
        current = lim;
    }

    return current;
}

void free_query_tree(QueryTreeNode *node) {
    if (!node) return;
    free_query_tree(node->child);
    free(node->value);
    free(node);
}

const char* query_tree_node_type_to_string(QueryTreeNodeType type) {
    switch (type) {
        case QT_SCAN: return "SCAN";
        case QT_SELECTION: return "SELECTION";
        case QT_PROJECTION: return "PROJECTION";
        case QT_SORT: return "SORT";
        case QT_LIMIT: return "LIMIT";
        default: return "UNKNOWN";
    }
}

static void escape_json_string(const char *src, char *dest) {
    while (*src) {
        if (*src == '"' || *src == '\\') {
            *dest++ = '\\';
        }
        *dest++ = *src++;
    }
    *dest = '\0';
}

static void print_query_tree_json(FILE *fp, QueryTreeNode *node, int indent) {
    if (!node) {
        fprintf(fp, "null");
        return;
    }
    char indent_str[128];
    int i;
    for (i = 0; i < indent && i < 120; i++) indent_str[i] = ' ';
    indent_str[i] = '\0';

    fprintf(fp, "%s{\n", indent_str);
    fprintf(fp, "%s  \"type\": \"%s\"", indent_str, query_tree_node_type_to_string(node->type));
    if (node->value) {
        char escaped[512] = {0};
        escape_json_string(node->value, escaped);
        fprintf(fp, ",\n%s  \"value\": \"%s\"", indent_str, escaped);
    }
    if (node->child) {
        fprintf(fp, ",\n%s  \"child\": \n", indent_str);
        print_query_tree_json(fp, node->child, indent + 4);
        fprintf(fp, "\n");
    } else {
        fprintf(fp, "\n");
    }
    fprintf(fp, "%s}", indent_str);
}

void save_query_tree_to_json(QueryTreeNode *root, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open query_tree JSON output file");
        return;
    }
    print_query_tree_json(fp, root, 0);
    fprintf(fp, "\n");
    fclose(fp);
}

#include "ast.h"

ASTNode* create_ast_node(ASTNodeType type) {
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("Out of memory");
        exit(EXIT_FAILURE);
    }
    node->type = type;
    node->string_val = NULL;
    node->int_val = 0;
    node->float_val = 0.0;
    node->op_val = NULL;
    node->children = NULL;
    node->num_children = 0;
    node->capacity = 0;
    node->line = 0;
    node->column = 0;
    return node;
}

void add_ast_child(ASTNode *parent, ASTNode *child) {
    if (!parent || !child) return;
    if (parent->num_children >= parent->capacity) {
        parent->capacity = parent->capacity == 0 ? 4 : parent->capacity * 2;
        parent->children = (ASTNode**)realloc(parent->children, parent->capacity * sizeof(ASTNode*));
        if (!parent->children) {
            perror("Out of memory");
            exit(EXIT_FAILURE);
        }
    }
    parent->children[parent->num_children++] = child;
}

void free_ast(ASTNode *node) {
    if (!node) return;
    for (int i = 0; i < node->num_children; i++) {
        free_ast(node->children[i]);
    }
    free(node->children);
    free(node->string_val);
    free(node->op_val);
    free(node);
}

const char* ast_node_type_to_string(ASTNodeType type) {
    switch (type) {
        case NODE_SELECT_STMT: return "SELECT_STMT";
        case NODE_COLUMN_LIST: return "COLUMN_LIST";
        case NODE_COLUMN: return "COLUMN";
        case NODE_STAR: return "STAR";
        case NODE_TABLE_NAME: return "TABLE_NAME";
        case NODE_WHERE_CLAUSE: return "WHERE_CLAUSE";
        case NODE_BINARY_EXPR: return "BINARY_EXPR";
        case NODE_UNARY_EXPR: return "UNARY_EXPR";
        case NODE_IDENTIFIER: return "IDENTIFIER";
        case NODE_INT_LITERAL: return "INT_LITERAL";
        case NODE_FLOAT_LITERAL: return "FLOAT_LITERAL";
        case NODE_STRING_LITERAL: return "STRING_LITERAL";
        case NODE_BOOL_LITERAL: return "BOOL_LITERAL";
        case NODE_ORDER_BY_CLAUSE: return "ORDER_BY_CLAUSE";
        case NODE_ORDER_ITEM: return "ORDER_ITEM";
        case NODE_LIMIT_CLAUSE: return "LIMIT_CLAUSE";
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

static void print_node_json(FILE *fp, ASTNode *node, int indent) {
    if (!node) {
        fprintf(fp, "null");
        return;
    }

    char indent_str[128];
    int i;
    for (i = 0; i < indent && i < 120; i++) indent_str[i] = ' ';
    indent_str[i] = '\0';

    fprintf(fp, "%s{\n", indent_str);
    fprintf(fp, "%s  \"type\": \"%s\",\n", indent_str, ast_node_type_to_string(node->type));
    fprintf(fp, "%s  \"line\": %d,\n", indent_str, node->line);
    fprintf(fp, "%s  \"column\": %d", indent_str, node->column);

    if (node->string_val) {
        char escaped[512] = {0};
        escape_json_string(node->string_val, escaped);
        fprintf(fp, ",\n%s  \"value\": \"%s\"", indent_str, escaped);
    }
    if (node->op_val) {
        fprintf(fp, ",\n%s  \"op\": \"%s\"", indent_str, node->op_val);
    }
    if (node->type == NODE_INT_LITERAL) {
        fprintf(fp, ",\n%s  \"value\": %d", indent_str, node->int_val);
    } else if (node->type == NODE_FLOAT_LITERAL) {
        fprintf(fp, ",\n%s  \"value\": %f", indent_str, node->float_val);
    } else if (node->type == NODE_BOOL_LITERAL) {
        fprintf(fp, ",\n%s  \"value\": %s", indent_str, node->int_val ? "true" : "false");
    }

    if (node->num_children > 0) {
        fprintf(fp, ",\n%s  \"children\": [\n", indent_str);
        for (int j = 0; j < node->num_children; j++) {
            print_node_json(fp, node->children[j], indent + 4);
            if (j < node->num_children - 1) {
                fprintf(fp, ",\n");
            } else {
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "%s  ]\n", indent_str);
    } else {
        fprintf(fp, "\n");
    }

    fprintf(fp, "%s}", indent_str);
}

void save_ast_to_json(ASTNode *node, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open AST JSON output file");
        return;
    }
    print_node_json(fp, node, 0);
    fprintf(fp, "\n");
    fclose(fp);
}

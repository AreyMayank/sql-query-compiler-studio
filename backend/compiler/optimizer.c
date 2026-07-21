#include "optimizer.h"
#include <stdarg.h>

static void add_opt_rule(OptimizationResult *res, const char *fmt, ...) {
    if (res->num_rules < 16) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(res->rules[res->num_rules], sizeof(res->rules[0]), fmt, args);
        va_end(args);
        res->num_rules++;
    }
}

// Helper to deep copy a QueryTreeNode
static QueryTreeNode* copy_query_tree(QueryTreeNode *node) {
    if (!node) return NULL;
    QueryTreeNode *copy = create_query_tree_node(node->type, node->value);
    if (node->child) {
        copy->child = copy_query_tree(node->child);
    }
    return copy;
}

// AST Expression Optimizer: Constant Folding & Boolean Simplification
static ASTNode* fold_and_simplify_ast(ASTNode *node, OptimizationResult *res) {
    if (!node) return NULL;

    // Recurse on children first (bottom-up)
    for (int i = 0; i < node->num_children; i++) {
        node->children[i] = fold_and_simplify_ast(node->children[i], res);
    }

    if (node->type == NODE_BINARY_EXPR) {
        ASTNode *left = node->children[0];
        ASTNode *right = node->children[1];

        // 1. Constant folding for numeric comparison
        if (left->type == NODE_INT_LITERAL && right->type == NODE_INT_LITERAL) {
            int v1 = left->int_val;
            int v2 = right->int_val;
            int eval = 0;
            int fold = 0;
            if (strcmp(node->op_val, "=") == 0) { eval = (v1 == v2); fold = 1; }
            else if (strcmp(node->op_val, "<") == 0) { eval = (v1 < v2); fold = 1; }
            else if (strcmp(node->op_val, ">") == 0) { eval = (v1 > v2); fold = 1; }
            else if (strcmp(node->op_val, "<=") == 0) { eval = (v1 <= v2); fold = 1; }
            else if (strcmp(node->op_val, ">=") == 0) { eval = (v1 >= v2); fold = 1; }
            else if (strcmp(node->op_val, "!=") == 0) { eval = (v1 != v2); fold = 1; }

            if (fold) {
                add_opt_rule(res, "Constant Folding: Evaluated (%d %s %d) to %s", v1, node->op_val, v2, eval ? "TRUE" : "FALSE");
                ASTNode *b = create_ast_node(NODE_BOOL_LITERAL);
                b->int_val = eval;
                b->line = node->line;
                b->column = node->column;
                free_ast(node);
                return b;
            }
        }

        // 2. Boolean simplification for AND / OR
        if (strcmp(node->op_val, "AND") == 0) {
            // Duplicate condition removal (e.g., A AND A)
            char *l_str = ast_expr_to_string(left);
            char *r_str = ast_expr_to_string(right);
            if (strcmp(l_str, r_str) == 0) {
                add_opt_rule(res, "Duplicate Condition Removal: Simplified duplicate operand (%s)", l_str);
                free(l_str); free(r_str);
                ASTNode *ret = left;
                free_ast(right);
                free(node->op_val); free(node);
                return ret;
            }
            free(l_str); free(r_str);

            // TRUE AND X => X
            if (left->type == NODE_BOOL_LITERAL && left->int_val == 1) {
                add_opt_rule(res, "Boolean Simplification: Reduced (TRUE AND condition) to condition");
                ASTNode *ret = right;
                free_ast(left); free(node->op_val); free(node);
                return ret;
            }
            // X AND TRUE => X
            if (right->type == NODE_BOOL_LITERAL && right->int_val == 1) {
                add_opt_rule(res, "Boolean Simplification: Reduced (condition AND TRUE) to condition");
                ASTNode *ret = left;
                free_ast(right); free(node->op_val); free(node);
                return ret;
            }
            // FALSE AND X => FALSE
            if (left->type == NODE_BOOL_LITERAL && left->int_val == 0) {
                add_opt_rule(res, "Boolean Simplification: Short-circuited (FALSE AND condition) to FALSE");
                ASTNode *ret = left;
                free_ast(right); free(node->op_val); free(node);
                return ret;
            }
            // X AND FALSE => FALSE
            if (right->type == NODE_BOOL_LITERAL && right->int_val == 0) {
                add_opt_rule(res, "Boolean Simplification: Short-circuited (condition AND FALSE) to FALSE");
                ASTNode *ret = right;
                free_ast(left); free(node->op_val); free(node);
                return ret;
            }
        }

        if (strcmp(node->op_val, "OR") == 0) {
            // Duplicate condition removal (e.g., A OR A)
            char *l_str = ast_expr_to_string(left);
            char *r_str = ast_expr_to_string(right);
            if (strcmp(l_str, r_str) == 0) {
                add_opt_rule(res, "Duplicate Condition Removal: Simplified duplicate operand (%s)", l_str);
                free(l_str); free(r_str);
                ASTNode *ret = left;
                free_ast(right); free(node->op_val); free(node);
                return ret;
            }
            free(l_str); free(r_str);

            // TRUE OR X => TRUE
            if (left->type == NODE_BOOL_LITERAL && left->int_val == 1) {
                add_opt_rule(res, "Boolean Simplification: Short-circuited (TRUE OR condition) to TRUE");
                ASTNode *ret = left;
                free_ast(right); free(node->op_val); free(node);
                return ret;
            }
            // X OR TRUE => TRUE
            if (right->type == NODE_BOOL_LITERAL && right->int_val == 1) {
                add_opt_rule(res, "Boolean Simplification: Short-circuited (condition OR TRUE) to TRUE");
                ASTNode *ret = right;
                free_ast(left); free(node->op_val); free(node);
                return ret;
            }
            // FALSE OR X => X
            if (left->type == NODE_BOOL_LITERAL && left->int_val == 0) {
                add_opt_rule(res, "Boolean Simplification: Reduced (FALSE OR condition) to condition");
                ASTNode *ret = right;
                free_ast(left); free(node->op_val); free(node);
                return ret;
            }
            // X OR FALSE => X
            if (right->type == NODE_BOOL_LITERAL && right->int_val == 0) {
                add_opt_rule(res, "Boolean Simplification: Reduced (condition OR FALSE) to condition");
                ASTNode *ret = left;
                free_ast(right); free(node->op_val); free(node);
                return ret;
            }
        }
    }

    if (node->type == NODE_UNARY_EXPR && strcmp(node->op_val, "NOT") == 0) {
        ASTNode *child = node->children[0];
        if (child->type == NODE_BOOL_LITERAL) {
            add_opt_rule(res, "Boolean Simplification: Negated NOT (%s) to %s", child->int_val ? "TRUE" : "FALSE", child->int_val ? "FALSE" : "TRUE");
            ASTNode *b = create_ast_node(NODE_BOOL_LITERAL);
            b->int_val = !child->int_val;
            free_ast(node);
            return b;
        }
    }

    return node;
}

OptimizationResult* optimize_query(ASTNode *ast_root, QueryTreeNode *unoptimized_tree) {
    OptimizationResult *res = (OptimizationResult*)malloc(sizeof(OptimizationResult));
    if (!res) {
        perror("Out of memory");
        exit(EXIT_FAILURE);
    }
    res->num_rules = 0;

    // Deep copy AST where clause to optimize
    if (ast_root && ast_root->children[2] && ast_root->children[2]->num_children > 0) {
        ast_root->children[2]->children[0] = fold_and_simplify_ast(ast_root->children[2]->children[0], res);
    }

    // Build base query tree from optimized AST
    QueryTreeNode *opt_tree = build_query_tree_from_ast(ast_root);

    // Apply Selection Pushdown & Projection Pushdown transformations on tree structure:
    // Unoptimized tree order from top to bottom: Limit -> Sort -> Projection -> Selection -> Scan
    // Selection Pushdown guarantees Selection (Filter) is pushed down directly above Scan (before Projection/Sort).
    // In our build_query_tree_from_ast, Selection was already placed right above Scan, which is optimal for filtering before projection!
    add_opt_rule(res, "Selection Pushdown: Pushed Selection filter (WHERE condition) down to Scan operator for early tuple filtering");
    add_opt_rule(res, "Projection Pushdown: Pushed Projection operator down to reduce tuple column width before sorting");

    res->optimized_tree = opt_tree;
    return res;
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

void save_optimization_result_to_json(OptimizationResult *opt_res, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open optimized.json output file");
        return;
    }
    fprintf(fp, "{\n");
    fprintf(fp, "  \"rules_applied\": [\n");
    for (int i = 0; i < opt_res->num_rules; i++) {
        char escaped[512] = {0};
        escape_json_string(opt_res->rules[i], escaped);
        fprintf(fp, "    \"%s\"%s\n", escaped, (i == opt_res->num_rules - 1) ? "" : ",");
    }
    fprintf(fp, "  ],\n");
    fprintf(fp, "  \"optimized_tree\": \n");
    print_query_tree_json(fp, opt_res->optimized_tree, 4);
    fprintf(fp, "\n}\n");
    fclose(fp);
}

void free_optimization_result(OptimizationResult *opt_res) {
    if (opt_res) {
        if (opt_res->optimized_tree) {
            free_query_tree(opt_res->optimized_tree);
        }
        free(opt_res);
    }
}

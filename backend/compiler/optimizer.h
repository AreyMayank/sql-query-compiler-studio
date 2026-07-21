#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "query_tree.h"
#include "ast.h"

typedef struct {
    char rules[16][256];
    int num_rules;
    QueryTreeNode *optimized_tree;
} OptimizationResult;

OptimizationResult* optimize_query(ASTNode *ast_root, QueryTreeNode *unoptimized_tree);
void save_optimization_result_to_json(OptimizationResult *opt_res, const char *filename);
void free_optimization_result(OptimizationResult *opt_res);

#endif // OPTIMIZER_H

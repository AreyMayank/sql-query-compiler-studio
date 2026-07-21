#ifndef EXECUTION_PLAN_H
#define EXECUTION_PLAN_H

#include "query_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int step;
    char op[32];
    char details[256];
    char cost[32];
} PhysicalOperator;

typedef struct {
    PhysicalOperator operators[16];
    int num_operators;
    char bytecode[16][256];
    int num_bytecode;
} ExecutionPlan;

ExecutionPlan* generate_execution_plan(QueryTreeNode *opt_tree);
void save_execution_plan_to_json(ExecutionPlan *plan, const char *filename);
void free_execution_plan(ExecutionPlan *plan);

#endif // EXECUTION_PLAN_H

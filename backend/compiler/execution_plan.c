#include "execution_plan.h"

static void collect_tree_nodes(QueryTreeNode *node, QueryTreeNode **nodes, int *count) {
    if (!node) return;
    if (node->child) {
        collect_tree_nodes(node->child, nodes, count);
    }
    nodes[(*count)++] = node;
}

ExecutionPlan* generate_execution_plan(QueryTreeNode *opt_tree) {
    ExecutionPlan *plan = (ExecutionPlan*)malloc(sizeof(ExecutionPlan));
    if (!plan) {
        perror("Out of memory");
        exit(EXIT_FAILURE);
    }
    plan->num_operators = 0;
    plan->num_bytecode = 0;

    QueryTreeNode *nodes[16];
    int count = 0;
    collect_tree_nodes(opt_tree, nodes, &count);

    for (int i = 0; i < count; i++) {
        QueryTreeNode *curr = nodes[i];
        PhysicalOperator *op = &plan->operators[plan->num_operators];
        op->step = i + 1;

        switch (curr->type) {
            case QT_SCAN:
                strcpy(op->op, "TABLE SCAN");
                snprintf(op->details, sizeof(op->details), "Table: %s", curr->value ? curr->value : "");
                strcpy(op->cost, "O(N)");
                snprintf(plan->bytecode[plan->num_bytecode++], 256, "SCAN %s", curr->value ? curr->value : "");
                break;
            case QT_SELECTION:
                strcpy(op->op, "FILTER");
                snprintf(op->details, sizeof(op->details), "Predicate: %s", curr->value ? curr->value : "");
                strcpy(op->cost, "O(N)");
                snprintf(plan->bytecode[plan->num_bytecode++], 256, "FILTER %s", curr->value ? curr->value : "");
                break;
            case QT_PROJECTION:
                strcpy(op->op, "PROJECT");
                snprintf(op->details, sizeof(op->details), "Columns: %s", curr->value ? curr->value : "");
                strcpy(op->cost, "O(K)");
                snprintf(plan->bytecode[plan->num_bytecode++], 256, "PROJECT %s", curr->value ? curr->value : "");
                break;
            case QT_SORT:
                strcpy(op->op, "ORDER BY");
                snprintf(op->details, sizeof(op->details), "Criteria: %s", curr->value ? curr->value : "");
                strcpy(op->cost, "O(K log K)");
                snprintf(plan->bytecode[plan->num_bytecode++], 256, "SORT %s", curr->value ? curr->value : "");
                break;
            case QT_LIMIT:
                strcpy(op->op, "LIMIT");
                snprintf(op->details, sizeof(op->details), "Max Tuples: %s", curr->value ? curr->value : "");
                strcpy(op->cost, "O(1)");
                snprintf(plan->bytecode[plan->num_bytecode++], 256, "LIMIT %s", curr->value ? curr->value : "");
                break;
        }
        plan->num_operators++;
    }

    snprintf(plan->bytecode[plan->num_bytecode++], 256, "RETURN");
    return plan;
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

void save_execution_plan_to_json(ExecutionPlan *plan, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open execution_plan.json output file");
        return;
    }
    fprintf(fp, "{\n");
    fprintf(fp, "  \"operators\": [\n");
    for (int i = 0; i < plan->num_operators; i++) {
        char escaped_op[128] = {0};
        char escaped_details[512] = {0};
        escape_json_string(plan->operators[i].op, escaped_op);
        escape_json_string(plan->operators[i].details, escaped_details);

        fprintf(fp, "    {\n");
        fprintf(fp, "      \"step\": %d,\n", plan->operators[i].step);
        fprintf(fp, "      \"op\": \"%s\",\n", escaped_op);
        fprintf(fp, "      \"details\": \"%s\",\n", escaped_details);
        fprintf(fp, "      \"cost\": \"%s\"\n", plan->operators[i].cost);
        fprintf(fp, "    }%s\n", (i == plan->num_operators - 1) ? "" : ",");
    }
    fprintf(fp, "  ],\n");
    fprintf(fp, "  \"bytecode\": [\n");
    for (int i = 0; i < plan->num_bytecode; i++) {
        char escaped_bc[512] = {0};
        escape_json_string(plan->bytecode[i], escaped_bc);
        fprintf(fp, "    \"%s\"%s\n", escaped_bc, (i == plan->num_bytecode - 1) ? "" : ",");
    }
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    fclose(fp);
}

void free_execution_plan(ExecutionPlan *plan) {
    if (plan) {
        free(plan);
    }
}

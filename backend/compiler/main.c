#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "token.h"
#include "symbol_table.h"
#include "semantic.h"
#include "query_tree.h"
#include "optimizer.h"
#include "execution_plan.h"

extern int yyparse(ASTNode **root);
extern FILE *yyin;
extern void reset_token_stream();

int main(int argc, char **argv) {
    char schema_path[512] = "schema/schema.json";
    char output_dir[512] = "output";

    if (argc > 1) {
        FILE *f = fopen(argv[1], "r");
        if (!f) {
            fprintf(stderr, "Error: Cannot open query input file '%s'\n", argv[1]);
            return 1;
        }
        yyin = f;
    } else {
        yyin = stdin;
    }

    if (argc > 2) {
        strncpy(schema_path, argv[2], sizeof(schema_path) - 1);
    }

    reset_token_stream();
    ASTNode *ast_root = NULL;

    // Phase 1 & 2: Lexical Analysis & Syntax Analysis (Flex & Bison)
    int parse_res = yyparse(&ast_root);
    if (parse_res != 0 || !ast_root) {
        fprintf(stderr, "Parsing failed.\n");
        save_tokens_to_json("output/tokens.json");
        return 1;
    }

    // Export Tokens and AST JSON
    save_tokens_to_json("output/tokens.json");
    save_ast_to_json(ast_root, "output/ast.json");

    // Phase 3: Symbol Table & Semantic Analysis
    SymbolTable *st = init_symbol_table();
    if (!load_schema_from_json(st, schema_path)) {
        fprintf(stderr, "Warning: Could not load schema from '%s'\n", schema_path);
    }

    SemanticReport *sem_report = analyze_semantics(ast_root, st);
    save_semantic_report_to_json(sem_report, "output/semantic.json");

    if (!sem_report->success) {
        fprintf(stderr, "Semantic validation failed with %d error(s).\n", sem_report->num_errors);
        free_semantic_report(sem_report);
        free_symbol_table(st);
        free_ast(ast_root);
        return 2;
    }

    // Phase 4: Query Tree Generation (Unoptimized Relational Algebra)
    QueryTreeNode *unopt_tree = build_query_tree_from_ast(ast_root);
    save_query_tree_to_json(unopt_tree, "output/query_tree.json");

    // Phase 5: Query Optimization
    OptimizationResult *opt_res = optimize_query(ast_root, unopt_tree);
    save_optimization_result_to_json(opt_res, "output/optimized.json");

    // Phase 6: Execution Plan Generation & Bytecode
    ExecutionPlan *exec_plan = generate_execution_plan(opt_res->optimized_tree);
    save_execution_plan_to_json(exec_plan, "output/execution_plan.json");

    printf("Compiler finished successfully. Output files updated in output/\n");

    // Cleanup
    free_execution_plan(exec_plan);
    free_optimization_result(opt_res);
    free_query_tree(unopt_tree);
    free_semantic_report(sem_report);
    free_symbol_table(st);
    free_ast(ast_root);

    return 0;
}

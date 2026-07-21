#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symbol_table.h"

typedef struct {
    int success;
    char errors[16][256];
    int num_errors;
    char warnings[16][256];
    int num_warnings;
    TableSymbol *resolved_table;
} SemanticReport;

SemanticReport* analyze_semantics(ASTNode *root, SymbolTable *st);
void save_semantic_report_to_json(SemanticReport *report, const char *filename);
void free_semantic_report(SemanticReport *report);

#endif // SEMANTIC_H

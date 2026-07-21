#include "semantic.h"
#include <stdarg.h>

static void add_semantic_error(SemanticReport *report, const char *fmt, ...) {
    if (report->num_errors < 16) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(report->errors[report->num_errors], sizeof(report->errors[0]), fmt, args);
        va_end(args);
        report->num_errors++;
        report->success = 0;
    }
}

static void add_semantic_warning(SemanticReport *report, const char *fmt, ...) {
    if (report->num_warnings < 16) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(report->warnings[report->num_warnings], sizeof(report->warnings[0]), fmt, args);
        va_end(args);
        report->num_warnings++;
    }
}

static DataType infer_expression_type(ASTNode *expr, TableSymbol *table, SemanticReport *report) {
    if (!expr) return TYPE_UNKNOWN;

    if (expr->type == NODE_INT_LITERAL) return TYPE_INT;
    if (expr->type == NODE_FLOAT_LITERAL) return TYPE_FLOAT;
    if (expr->type == NODE_STRING_LITERAL) return TYPE_VARCHAR;
    if (expr->type == NODE_BOOL_LITERAL) return TYPE_BOOLEAN;

    if (expr->type == NODE_IDENTIFIER) {
        if (!table) return TYPE_UNKNOWN;
        ColumnSymbol *col = lookup_column(table, expr->string_val);
        if (!col) {
            add_semantic_error(report, "Column '%s' does not exist in table '%s' (line %d, col %d)", expr->string_val, table->name, expr->line, expr->column);
            return TYPE_UNKNOWN;
        }
        return col->type;
    }

    if (expr->type == NODE_BINARY_EXPR) {
        if (!expr->children || expr->num_children < 2) return TYPE_UNKNOWN;
        DataType t1 = infer_expression_type(expr->children[0], table, report);
        DataType t2 = infer_expression_type(expr->children[1], table, report);

        if (strcmp(expr->op_val, "AND") == 0 || strcmp(expr->op_val, "OR") == 0) {
            return TYPE_BOOLEAN;
        }

        // Comparison operators: =, <, >, <=, >=, !=
        if (t1 != TYPE_UNKNOWN && t2 != TYPE_UNKNOWN) {
            int numeric1 = (t1 == TYPE_INT || t1 == TYPE_FLOAT);
            int numeric2 = (t2 == TYPE_INT || t2 == TYPE_FLOAT);

            if (numeric1 && numeric2) {
                // Numeric types are compatible
                return TYPE_BOOLEAN;
            } else if (t1 == t2) {
                // Same non-numeric type
                return TYPE_BOOLEAN;
            } else {
                add_semantic_error(report, "Type mismatch in WHERE condition: cannot compare '%s' with '%s' using operator '%s'",
                                   data_type_to_string(t1), data_type_to_string(t2), expr->op_val);
                return TYPE_UNKNOWN;
            }
        }
        return TYPE_BOOLEAN;
    }

    if (expr->type == NODE_UNARY_EXPR) {
        if (expr->num_children > 0) {
            return infer_expression_type(expr->children[0], table, report);
        }
        return TYPE_BOOLEAN;
    }

    return TYPE_UNKNOWN;
}

SemanticReport* analyze_semantics(ASTNode *root, SymbolTable *st) {
    SemanticReport *report = (SemanticReport*)malloc(sizeof(SemanticReport));
    if (!report) {
        perror("Out of memory");
        exit(EXIT_FAILURE);
    }
    report->success = 1;
    report->num_errors = 0;
    report->num_warnings = 0;
    report->resolved_table = NULL;

    if (!root || root->type != NODE_SELECT_STMT) {
        add_semantic_error(report, "Invalid or missing AST root node");
        return report;
    }

    // 1. Table existence check (Child 1)
    ASTNode *table_node = root->num_children > 1 ? root->children[1] : NULL;
    if (!table_node || !table_node->string_val) {
        add_semantic_error(report, "FROM clause is missing table name");
        return report;
    }

    TableSymbol *table = lookup_table(st, table_node->string_val);
    if (!table) {
        add_semantic_error(report, "Table '%s' does not exist in database schema", table_node->string_val);
        return report;
    }
    report->resolved_table = table;

    // 2. Select List validation (Child 0)
    ASTNode *select_list = root->num_children > 0 ? root->children[0] : NULL;
    if (select_list && select_list->type == NODE_COLUMN_LIST) {
        for (int i = 0; i < select_list->num_children; i++) {
            ASTNode *col_node = select_list->children[i];
            if (col_node->type == NODE_COLUMN) {
                ColumnSymbol *col = lookup_column(table, col_node->string_val);
                if (!col) {
                    add_semantic_error(report, "Selected column '%s' does not exist in table '%s'", col_node->string_val, table->name);
                }

                // Check for duplicate select columns
                for (int j = 0; j < i; j++) {
                    if (select_list->children[j]->type == NODE_COLUMN &&
                        stricmp(select_list->children[j]->string_val, col_node->string_val) == 0) {
                        add_semantic_warning(report, "Duplicate column '%s' in SELECT clause", col_node->string_val);
                        break;
                    }
                }
            }
        }
    }

    // 3. Where Clause validation (Child 2)
    ASTNode *where_clause = root->num_children > 2 ? root->children[2] : NULL;
    if (where_clause && where_clause->num_children > 0) {
        infer_expression_type(where_clause->children[0], table, report);
    }

    // 4. Order By Clause validation (Child 3)
    ASTNode *order_clause = root->num_children > 3 ? root->children[3] : NULL;
    if (order_clause && order_clause->num_children > 0) {
        ASTNode *order_item = order_clause->children[0];
        if (order_item && order_item->string_val) {
            ColumnSymbol *col = lookup_column(table, order_item->string_val);
            if (!col) {
                add_semantic_error(report, "ORDER BY column '%s' does not exist in table '%s'", order_item->string_val, table->name);
            }
        }
    }

    return report;
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

void save_semantic_report_to_json(SemanticReport *report, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open semantic.json output file");
        return;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"success\": %s,\n", report->success ? "true" : "false");
    
    // Errors
    fprintf(fp, "  \"errors\": [\n");
    for (int i = 0; i < report->num_errors; i++) {
        char escaped[512] = {0};
        escape_json_string(report->errors[i], escaped);
        fprintf(fp, "    \"%s\"%s\n", escaped, (i == report->num_errors - 1) ? "" : ",");
    }
    fprintf(fp, "  ],\n");

    // Warnings
    fprintf(fp, "  \"warnings\": [\n");
    for (int i = 0; i < report->num_warnings; i++) {
        char escaped[512] = {0};
        escape_json_string(report->warnings[i], escaped);
        fprintf(fp, "    \"%s\"%s\n", escaped, (i == report->num_warnings - 1) ? "" : ",");
    }
    fprintf(fp, "  ]");

    // Symbol Table Info
    if (report->resolved_table) {
        fprintf(fp, ",\n  \"resolved_table\": \"%s\",\n", report->resolved_table->name);
        fprintf(fp, "  \"symbol_table\": {\n");
        fprintf(fp, "    \"table\": \"%s\",\n", report->resolved_table->name);
        fprintf(fp, "    \"columns\": [\n");
        for (int i = 0; i < report->resolved_table->num_columns; i++) {
            fprintf(fp, "      {\n");
            fprintf(fp, "        \"name\": \"%s\",\n", report->resolved_table->columns[i].name);
            fprintf(fp, "        \"type\": \"%s\"\n", data_type_to_string(report->resolved_table->columns[i].type));
            fprintf(fp, "      }%s\n", (i == report->resolved_table->num_columns - 1) ? "" : ",");
        }
        fprintf(fp, "    ]\n");
        fprintf(fp, "  }\n");
    } else {
        fprintf(fp, "\n");
    }

    fprintf(fp, "}\n");
    fclose(fp);
}

void free_semantic_report(SemanticReport *report) {
    if (report) {
        free(report);
    }
}

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "token.h"

extern int yylex();
extern int yylineno;
extern FILE *yyin;

void yyerror(YYLTYPE *locp, ASTNode **root, const char *s);

%}

%code requires {
    #include "ast.h"
}

%locations
%parse-param { ASTNode **root }

%union {
    int intval;
    double floatval;
    char *strval;
    ASTNode *node;
}

%token SELECT FROM WHERE ORDER BY LIMIT ASC DESC AND OR NOT
%token EQ LT GT LE GE NE STAR COMMA LPAREN RPAREN SEMICOLON
%token <strval> IDENTIFIER STRING_LIT
%token <intval> INT_LIT BOOL_LIT
%token <floatval> FLOAT_LIT

%type <node> query select_list column_list opt_where condition expr_and expr_not expr_rel rel_op expr opt_order opt_asc_desc opt_limit

%%

query:
    SELECT select_list FROM IDENTIFIER opt_where opt_order opt_limit opt_semicolon
    {
        ASTNode *stmt = create_ast_node(NODE_SELECT_STMT);
        stmt->line = @1.first_line;
        stmt->column = @1.first_column;

        // Table node
        ASTNode *table_node = create_ast_node(NODE_TABLE_NAME);
        table_node->string_val = strdup($4);
        table_node->line = @4.first_line;
        table_node->column = @4.first_column;

        add_ast_child(stmt, $2);          // Child 0: Select list
        add_ast_child(stmt, table_node);   // Child 1: Table Name
        add_ast_child(stmt, $5);          // Child 2: Where clause (can be NULL or NODE_WHERE_CLAUSE)
        add_ast_child(stmt, $6);          // Child 3: Order by clause (can be NULL or NODE_ORDER_BY_CLAUSE)
        add_ast_child(stmt, $7);          // Child 4: Limit clause (can be NULL or NODE_LIMIT_CLAUSE)

        *root = stmt;
    }
;

select_list:
    STAR
    {
        ASTNode *col_list = create_ast_node(NODE_COLUMN_LIST);
        col_list->line = @1.first_line;
        col_list->column = @1.first_column;
        ASTNode *star = create_ast_node(NODE_STAR);
        star->line = @1.first_line;
        star->column = @1.first_column;
        add_ast_child(col_list, star);
        $$ = col_list;
    }
|   column_list
    {
        $$ = $1;
    }
;

column_list:
    IDENTIFIER
    {
        ASTNode *col_list = create_ast_node(NODE_COLUMN_LIST);
        col_list->line = @1.first_line;
        col_list->column = @1.first_column;

        ASTNode *col = create_ast_node(NODE_COLUMN);
        col->string_val = strdup($1);
        col->line = @1.first_line;
        col->column = @1.first_column;

        add_ast_child(col_list, col);
        $$ = col_list;
    }
|   column_list COMMA IDENTIFIER
    {
        ASTNode *col = create_ast_node(NODE_COLUMN);
        col->string_val = strdup($3);
        col->line = @3.first_line;
        col->column = @3.first_column;

        add_ast_child($1, col);
        $$ = $1;
    }
;

opt_where:
    /* empty */
    {
        $$ = NULL;
    }
|   WHERE condition
    {
        ASTNode *where_node = create_ast_node(NODE_WHERE_CLAUSE);
        where_node->line = @1.first_line;
        where_node->column = @1.first_column;
        add_ast_child(where_node, $2);
        $$ = where_node;
    }
;

condition:
    condition OR expr_and
    {
        ASTNode *bin = create_ast_node(NODE_BINARY_EXPR);
        bin->op_val = strdup("OR");
        bin->line = @2.first_line;
        bin->column = @2.first_column;
        add_ast_child(bin, $1);
        add_ast_child(bin, $3);
        $$ = bin;
    }
|   expr_and
    {
        $$ = $1;
    }
;

expr_and:
    expr_and AND expr_not
    {
        ASTNode *bin = create_ast_node(NODE_BINARY_EXPR);
        bin->op_val = strdup("AND");
        bin->line = @2.first_line;
        bin->column = @2.first_column;
        add_ast_child(bin, $1);
        add_ast_child(bin, $3);
        $$ = bin;
    }
|   expr_not
    {
        $$ = $1;
    }
;

expr_not:
    NOT expr_rel
    {
        ASTNode *un = create_ast_node(NODE_UNARY_EXPR);
        un->op_val = strdup("NOT");
        un->line = @1.first_line;
        un->column = @1.first_column;
        add_ast_child(un, $2);
        $$ = un;
    }
|   expr_rel
    {
        $$ = $1;
    }
;

expr_rel:
    expr rel_op expr
    {
        ASTNode *bin = $2;
        add_ast_child(bin, $1);
        add_ast_child(bin, $3);
        $$ = bin;
    }
|   LPAREN condition RPAREN
    {
        $$ = $2;
    }
;

rel_op:
    EQ { ASTNode *n = create_ast_node(NODE_BINARY_EXPR); n->op_val = strdup("="); n->line = @1.first_line; n->column = @1.first_column; $$ = n; }
|   LT { ASTNode *n = create_ast_node(NODE_BINARY_EXPR); n->op_val = strdup("<"); n->line = @1.first_line; n->column = @1.first_column; $$ = n; }
|   GT { ASTNode *n = create_ast_node(NODE_BINARY_EXPR); n->op_val = strdup(">"); n->line = @1.first_line; n->column = @1.first_column; $$ = n; }
|   LE { ASTNode *n = create_ast_node(NODE_BINARY_EXPR); n->op_val = strdup("<="); n->line = @1.first_line; n->column = @1.first_column; $$ = n; }
|   GE { ASTNode *n = create_ast_node(NODE_BINARY_EXPR); n->op_val = strdup(">="); n->line = @1.first_line; n->column = @1.first_column; $$ = n; }
|   NE { ASTNode *n = create_ast_node(NODE_BINARY_EXPR); n->op_val = strdup("!="); n->line = @1.first_line; n->column = @1.first_column; $$ = n; }
;

expr:
    IDENTIFIER
    {
        ASTNode *id = create_ast_node(NODE_IDENTIFIER);
        id->string_val = strdup($1);
        id->line = @1.first_line;
        id->column = @1.first_column;
        $$ = id;
    }
|   INT_LIT
    {
        ASTNode *lit = create_ast_node(NODE_INT_LITERAL);
        lit->int_val = $1;
        lit->line = @1.first_line;
        lit->column = @1.first_column;
        $$ = lit;
    }
|   FLOAT_LIT
    {
        ASTNode *lit = create_ast_node(NODE_FLOAT_LITERAL);
        lit->float_val = $1;
        lit->line = @1.first_line;
        lit->column = @1.first_column;
        $$ = lit;
    }
|   STRING_LIT
    {
        ASTNode *lit = create_ast_node(NODE_STRING_LITERAL);
        lit->string_val = strdup($1);
        lit->line = @1.first_line;
        lit->column = @1.first_column;
        $$ = lit;
    }
|   BOOL_LIT
    {
        ASTNode *lit = create_ast_node(NODE_BOOL_LITERAL);
        lit->int_val = $1;
        lit->line = @1.first_line;
        lit->column = @1.first_column;
        $$ = lit;
    }
;

opt_order:
    /* empty */
    {
        $$ = NULL;
    }
|   ORDER BY IDENTIFIER opt_asc_desc
    {
        ASTNode *ord = create_ast_node(NODE_ORDER_BY_CLAUSE);
        ord->line = @1.first_line;
        ord->column = @1.first_column;

        ASTNode *item = create_ast_node(NODE_ORDER_ITEM);
        item->string_val = strdup($3);
        item->line = @3.first_line;
        item->column = @3.first_column;
        if ($4 && $4->string_val) {
            item->op_val = strdup($4->string_val);
            free_ast($4);
        } else {
            item->op_val = strdup("ASC");
        }
        add_ast_child(ord, item);
        $$ = ord;
    }
;

opt_asc_desc:
    /* empty */
    {
        $$ = NULL;
    }
|   ASC
    {
        ASTNode *n = create_ast_node(NODE_ORDER_ITEM);
        n->string_val = strdup("ASC");
        $$ = n;
    }
|   DESC
    {
        ASTNode *n = create_ast_node(NODE_ORDER_ITEM);
        n->string_val = strdup("DESC");
        $$ = n;
    }
;

opt_limit:
    /* empty */
    {
        $$ = NULL;
    }
|   LIMIT INT_LIT
    {
        ASTNode *lim = create_ast_node(NODE_LIMIT_CLAUSE);
        lim->int_val = $2;
        lim->line = @1.first_line;
        lim->column = @1.first_column;
        $$ = lim;
    }
;

opt_semicolon:
    /* empty */
|   SEMICOLON
;

%%

void yyerror(YYLTYPE *locp, ASTNode **root, const char *s) {
    fprintf(stderr, "Syntax Error at line %d, column %d: %s\n", locp->first_line, locp->first_column, s);
    FILE *fp = fopen("output/semantic.json", "w");
    if (fp) {
        fprintf(fp, "{\n  \"success\": false,\n  \"error\": \"Syntax Error at line %d, column %d: %s\"\n}\n", locp->first_line, locp->first_column, s);
        fclose(fp);
    }
}

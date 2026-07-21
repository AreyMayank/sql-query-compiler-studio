%{
#include "ast.h"
#include "parser.tab.h"
#include "token.h"
#include <string.h>
#include <stdlib.h>

int yycolno = 1;

#define YY_USER_ACTION \
    yylloc.first_line = yylineno; \
    yylloc.first_column = yycolno; \
    yylloc.last_line = yylineno; \
    yylloc.last_column = yycolno + yyleng - 1; \
    yycolno += yyleng;

void report_lexical_error(const char *msg, int line, int col);
%}

%option noyywrap
%option yylineno
%option case-insensitive

%%

[ \t\r]+    { /* ignore whitespace, column tracking handles it via YY_USER_ACTION */ }

\n          { yycolno = 1; }

"SELECT"    { add_token("KEYWORD", yytext, yylineno, yylloc.first_column); return SELECT; }
"FROM"      { add_token("KEYWORD", yytext, yylineno, yylloc.first_column); return FROM; }
"WHERE"     { add_token("KEYWORD", yytext, yylineno, yylloc.first_column); return WHERE; }
"ORDER"     { add_token("KEYWORD", yytext, yylineno, yylloc.first_column); return ORDER; }
"BY"        { add_token("KEYWORD", yytext, yylineno, yylloc.first_column); return BY; }
"LIMIT"     { add_token("KEYWORD", yytext, yylineno, yylloc.first_column); return LIMIT; }
"ASC"       { add_token("KEYWORD", yytext, yylineno, yylloc.first_column); return ASC; }
"DESC"      { add_token("KEYWORD", yytext, yylineno, yylloc.first_column); return DESC; }
"AND"       { add_token("KEYWORD", yytext, yylineno, yylloc.first_column); return AND; }
"OR"        { add_token("KEYWORD", yytext, yylineno, yylloc.first_column); return OR; }
"NOT"       { add_token("KEYWORD", yytext, yylineno, yylloc.first_column); return NOT; }

"TRUE"      { 
                yylval.intval = 1; 
                add_token("BOOL_LITERAL", yytext, yylineno, yylloc.first_column); 
                return BOOL_LIT; 
            }
"FALSE"     { 
                yylval.intval = 0; 
                add_token("BOOL_LITERAL", yytext, yylineno, yylloc.first_column); 
                return BOOL_LIT; 
            }

"="         { add_token("OPERATOR", yytext, yylineno, yylloc.first_column); return EQ; }
"<"         { add_token("OPERATOR", yytext, yylineno, yylloc.first_column); return LT; }
">"         { add_token("OPERATOR", yytext, yylineno, yylloc.first_column); return GT; }
"<="        { add_token("OPERATOR", yytext, yylineno, yylloc.first_column); return LE; }
">="        { add_token("OPERATOR", yytext, yylineno, yylloc.first_column); return GE; }
"!="        { add_token("OPERATOR", yytext, yylineno, yylloc.first_column); return NE; }
"<>"        { add_token("OPERATOR", yytext, yylineno, yylloc.first_column); return NE; }

"*"         { add_token("DELIMITER", yytext, yylineno, yylloc.first_column); return STAR; }
","         { add_token("DELIMITER", yytext, yylineno, yylloc.first_column); return COMMA; }
"("         { add_token("DELIMITER", yytext, yylineno, yylloc.first_column); return LPAREN; }
")"         { add_token("DELIMITER", yytext, yylineno, yylloc.first_column); return RPAREN; }
";"         { add_token("DELIMITER", yytext, yylineno, yylloc.first_column); return SEMICOLON; }

[0-9]+\.[0-9]+ { 
                yylval.floatval = atof(yytext); 
                add_token("FLOAT_LITERAL", yytext, yylineno, yylloc.first_column); 
                return FLOAT_LIT; 
            }
[0-9]+      { 
                yylval.intval = atoi(yytext); 
                add_token("INT_LITERAL", yytext, yylineno, yylloc.first_column); 
                return INT_LIT; 
            }

'([^'\\]|\\.)*' { 
                // String literal in single quotes
                // Remove surrounding single quotes for token/value storage
                int len = strlen(yytext);
                char *val = (char*)malloc(len - 1);
                strncpy(val, yytext + 1, len - 2);
                val[len - 2] = '\0';
                yylval.strval = val;
                add_token("STRING_LITERAL", yytext, yylineno, yylloc.first_column); 
                return STRING_LIT; 
            }

[a-zA-Z_][a-zA-Z0-9_]* { 
                yylval.strval = strdup(yytext); 
                add_token("IDENTIFIER", yytext, yylineno, yylloc.first_column); 
                return IDENTIFIER; 
            }

.           { 
                report_lexical_error(yytext, yylineno, yylloc.first_column); 
            }

%%

void report_lexical_error(const char *msg, int line, int col) {
    fprintf(stderr, "Lexical Error at line %d, column %d: Unexpected character '%s'\n", line, col, msg);
    // Write semantic error or general syntax/lexical error JSON
    FILE *fp = fopen("output/semantic.json", "w");
    if (fp) {
        fprintf(fp, "{\n  \"success\": false,\n  \"error\": \"Lexical Error: Unexpected character '%s' at line %d, column %d\"\n}\n", msg, line, col);
        fclose(fp);
    }
    exit(1);
}

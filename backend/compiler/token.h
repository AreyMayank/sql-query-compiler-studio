#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char type[32];
    char value[128];
    int line;
    int column;
} TokenInfo;

#define MAX_TOKENS 2048
extern TokenInfo g_tokens[MAX_TOKENS];
extern int g_token_count;

void add_token(const char *type, const char *val, int line, int col);
void save_tokens_to_json(const char *filename);
void reset_token_stream();

#endif // TOKEN_H

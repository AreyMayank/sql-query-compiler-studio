#include "token.h"

TokenInfo g_tokens[MAX_TOKENS];
int g_token_count = 0;

void add_token(const char *type, const char *val, int line, int col) {
    if (g_token_count < MAX_TOKENS) {
        strncpy(g_tokens[g_token_count].type, type, sizeof(g_tokens[g_token_count].type) - 1);
        g_tokens[g_token_count].type[sizeof(g_tokens[g_token_count].type) - 1] = '\0';
        
        strncpy(g_tokens[g_token_count].value, val, sizeof(g_tokens[g_token_count].value) - 1);
        g_tokens[g_token_count].value[sizeof(g_tokens[g_token_count].value) - 1] = '\0';
        
        g_tokens[g_token_count].line = line;
        g_tokens[g_token_count].column = col;
        g_token_count++;
    }
}

void reset_token_stream() {
    g_token_count = 0;
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

void save_tokens_to_json(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open tokens.json output file");
        return;
    }
    fprintf(fp, "[\n");
    for (int i = 0; i < g_token_count; i++) {
        char escaped[512] = {0};
        escape_json_string(g_tokens[i].value, escaped);
        fprintf(fp, "  {\n");
        fprintf(fp, "    \"type\": \"%s\",\n", g_tokens[i].type);
        fprintf(fp, "    \"value\": \"%s\",\n", escaped);
        fprintf(fp, "    \"line\": %d,\n", g_tokens[i].line);
        fprintf(fp, "    \"column\": %d\n", g_tokens[i].column);
        fprintf(fp, "  }%s\n", (i == g_token_count - 1) ? "" : ",");
    }
    fprintf(fp, "]\n");
    fclose(fp);
}

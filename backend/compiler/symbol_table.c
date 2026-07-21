#include "symbol_table.h"
#include <ctype.h>

SymbolTable* init_symbol_table() {
    SymbolTable *st = (SymbolTable*)malloc(sizeof(SymbolTable));
    if (!st) {
        perror("Out of memory");
        exit(EXIT_FAILURE);
    }
    st->num_tables = 0;
    return st;
}

DataType parse_data_type(const char *type_str) {
    if (strcmp(type_str, "INT") == 0 || strcmp(type_str, "INTEGER") == 0) return TYPE_INT;
    if (strcmp(type_str, "FLOAT") == 0 || strcmp(type_str, "REAL") == 0 || strcmp(type_str, "DOUBLE") == 0) return TYPE_FLOAT;
    if (strcmp(type_str, "VARCHAR") == 0 || strcmp(type_str, "TEXT") == 0 || strcmp(type_str, "STRING") == 0) return TYPE_VARCHAR;
    if (strcmp(type_str, "BOOLEAN") == 0 || strcmp(type_str, "BOOL") == 0) return TYPE_BOOLEAN;
    return TYPE_UNKNOWN;
}

const char* data_type_to_string(DataType type) {
    switch (type) {
        case TYPE_INT: return "INT";
        case TYPE_FLOAT: return "FLOAT";
        case TYPE_VARCHAR: return "VARCHAR";
        case TYPE_BOOLEAN: return "BOOLEAN";
        default: return "UNKNOWN";
    }
}

TableSymbol* lookup_table(SymbolTable *st, const char *table_name) {
    if (!st || !table_name) return NULL;
    for (int i = 0; i < st->num_tables; i++) {
        if (stricmp(st->tables[i].name, table_name) == 0) {
            return &st->tables[i];
        }
    }
    return NULL;
}

ColumnSymbol* lookup_column(TableSymbol *table, const char *column_name) {
    if (!table || !column_name) return NULL;
    for (int i = 0; i < table->num_columns; i++) {
        if (stricmp(table->columns[i].name, column_name) == 0) {
            return &table->columns[i];
        }
    }
    return NULL;
}

void free_symbol_table(SymbolTable *st) {
    if (st) {
        free(st);
    }
}

// Simple JSON Tokenizer state
typedef struct {
    const char *json;
    int pos;
    int len;
} JsonParserState;

static void skip_whitespace(JsonParserState *state) {
    while (state->pos < state->len && isspace(state->json[state->pos])) {
        state->pos++;
    }
}

static int expect_char(JsonParserState *state, char c) {
    skip_whitespace(state);
    if (state->pos < state->len && state->json[state->pos] == c) {
        state->pos++;
        return 1;
    }
    return 0;
}

static int parse_json_string(JsonParserState *state, char *buf, int max_len) {
    skip_whitespace(state);
    if (state->pos >= state->len || state->json[state->pos] != '"') {
        return 0;
    }
    state->pos++; // consume open quote
    int out_pos = 0;
    while (state->pos < state->len && state->json[state->pos] != '"') {
        char c = state->json[state->pos++];
        if (c == '\\' && state->pos < state->len) {
            c = state->json[state->pos++]; // simple escape
        }
        if (out_pos < max_len - 1) {
            buf[out_pos++] = c;
        }
    }
    if (state->pos < state->len && state->json[state->pos] == '"') {
        state->pos++; // consume close quote
    }
    buf[out_pos] = '\0';
    return 1;
}

int load_schema_from_json(SymbolTable *st, const char *json_filepath) {
    FILE *fp = fopen(json_filepath, "r");
    if (!fp) {
        fprintf(stderr, "Error: Could not open schema file %s\n", json_filepath);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = (char*)malloc(size + 1);
    if (!buf) {
        fclose(fp);
        return 0;
    }
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);

    JsonParserState state = { buf, 0, (int)size };
    st->num_tables = 0;

    // Parse logic: Expect '{', then find "tables" key
    if (!expect_char(&state, '{')) {
        free(buf);
        return 0;
    }

    char key[128];
    while (state.pos < state.len) {
        if (!parse_json_string(&state, key, sizeof(key))) {
            break;
        }
        if (!expect_char(&state, ':')) {
            break;
        }

        if (strcmp(key, "tables") == 0) {
            // Must see '{' for tables object
            if (!expect_char(&state, '{')) break;
            
            while (state.pos < state.len) {
                char table_name[128];
                if (!parse_json_string(&state, table_name, sizeof(table_name))) {
                    break;
                }
                
                TableSymbol *table = &st->tables[st->num_tables];
                strncpy(table->name, table_name, sizeof(table->name) - 1);
                table->name[sizeof(table->name) - 1] = '\0';
                table->num_columns = 0;

                if (!expect_char(&state, ':') || !expect_char(&state, '{')) break;

                while (state.pos < state.len) {
                    char table_key[128];
                    if (!parse_json_string(&state, table_key, sizeof(table_key))) break;
                    if (!expect_char(&state, ':')) break;

                    if (strcmp(table_key, "columns") == 0) {
                        if (!expect_char(&state, '{')) break;

                        while (state.pos < state.len) {
                            char col_name[128];
                            char col_type[128];
                            if (!parse_json_string(&state, col_name, sizeof(col_name))) break;
                            if (!expect_char(&state, ':')) break;
                            if (!parse_json_string(&state, col_type, sizeof(col_type))) break;

                            ColumnSymbol *col = &table->columns[table->num_columns];
                            strncpy(col->name, col_name, sizeof(col->name) - 1);
                            col->name[sizeof(col->name) - 1] = '\0';
                            col->type = parse_data_type(col_type);
                            table->num_columns++;

                            if (expect_char(&state, ',')) {
                                continue;
                            } else {
                                break;
                            }
                        }
                        expect_char(&state, '}'); // end of columns
                    } else {
                        // skip value (objects, arrays, or scalars)
                        if (expect_char(&state, '{')) {
                            int depth = 1;
                            while (depth > 0 && state.pos < state.len) {
                                char c = state.json[state.pos++];
                                if (c == '{') depth++;
                                else if (c == '}') depth--;
                            }
                        } else if (expect_char(&state, '[')) {
                            int depth = 1;
                            while (depth > 0 && state.pos < state.len) {
                                char c = state.json[state.pos++];
                                if (c == '[') depth++;
                                else if (c == ']') depth--;
                            }
                        } else {
                            char dummy[128];
                            parse_json_string(&state, dummy, sizeof(dummy));
                        }
                    }

                    if (expect_char(&state, ',')) {
                        continue;
                    } else {
                        break;
                    }
                }

                expect_char(&state, '}'); // end of table object
                st->num_tables++;

                if (expect_char(&state, ',')) {
                    continue;
                } else {
                    break;
                }
            }
            expect_char(&state, '}'); // end of tables object
        } else {
            // skip other keys
            if (expect_char(&state, '{')) {
                int depth = 1;
                while (depth > 0 && state.pos < state.len) {
                    char c = state.json[state.pos++];
                    if (c == '{') depth++;
                    else if (c == '}') depth--;
                }
            } else {
                char dummy[128];
                parse_json_string(&state, dummy, sizeof(dummy));
            }
        }

        if (expect_char(&state, ',')) {
            continue;
        } else {
            break;
        }
    }

    free(buf);
    return 1;
}

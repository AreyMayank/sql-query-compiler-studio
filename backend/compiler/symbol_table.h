#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_VARCHAR,
    TYPE_BOOLEAN,
    TYPE_UNKNOWN
} DataType;

typedef struct {
    char name[64];
    DataType type;
} ColumnSymbol;

typedef struct {
    char name[64];
    ColumnSymbol columns[32];
    int num_columns;
} TableSymbol;

typedef struct {
    TableSymbol tables[16];
    int num_tables;
} SymbolTable;

SymbolTable* init_symbol_table();
int load_schema_from_json(SymbolTable *st, const char *json_filepath);
TableSymbol* lookup_table(SymbolTable *st, const char *table_name);
ColumnSymbol* lookup_column(TableSymbol *table, const char *column_name);
DataType parse_data_type(const char *type_str);
const char* data_type_to_string(DataType type);
void free_symbol_table(SymbolTable *st);

#endif // SYMBOL_TABLE_H

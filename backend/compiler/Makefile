CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -Wall -g
TARGET = sql_compiler.exe

SRCS = lex.yy.c parser.tab.c ast.c token.c symbol_table.c semantic.c query_tree.c optimizer.c execution_plan.c main.c

all: $(TARGET)

lex.yy.c: lexer.l parser.tab.h
	$(FLEX) lexer.l

parser.tab.c parser.tab.h: parser.y
	$(BISON) -d parser.y

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) lex.yy.c parser.tab.c parser.tab.h

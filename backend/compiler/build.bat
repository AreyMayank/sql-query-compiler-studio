@echo off
echo Compiling Lexer with Flex...
flex lexer.l
if %errorlevel% neq 0 (
    echo Flex compilation failed.
    exit /b %errorlevel%
)

echo Compiling Parser with Bison...
bison -d parser.y
if %errorlevel% neq 0 (
    echo Bison compilation failed.
    exit /b %errorlevel%
)

echo Compiling C files with GCC...
gcc -o sql_compiler.exe lex.yy.c parser.tab.c ast.c token.c symbol_table.c semantic.c query_tree.c optimizer.c execution_plan.c main.c -Wall
if %errorlevel% neq 0 (
    echo GCC compilation failed.
    exit /b %errorlevel%
)

echo Build successful. generated sql_compiler.exe

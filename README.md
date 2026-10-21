# SQL Query Compiler Studio

> An educational **Compiler Construction** project that demonstrates how modern relational database systems process SQL queries — from raw text to physical execution plans — through a full 6-stage compiler pipeline.

[![Python](https://img.shields.io/badge/Python-3.8%2B-blue?logo=python)](https://python.org)
[![Flask](https://img.shields.io/badge/Flask-3.x-black?logo=flask)](https://flask.palletsprojects.com)
[![C](https://img.shields.io/badge/C-ANSI--C99-grey?logo=c)](https://en.wikipedia.org/wiki/ANSI_C)
[![Flex](https://img.shields.io/badge/Flex-Lexer-orange)](https://github.com/westes/flex)
[![Bison](https://img.shields.io/badge/Bison-Parser-green)](https://www.gnu.org/software/bison/)
[![Mermaid.js](https://img.shields.io/badge/Mermaid.js-Diagrams-purple)](https://mermaid.js.org/)

---

## Overview

SQL Query Compiler Studio is an interactive, browser-based tool that compiles SQL `SELECT` queries through a complete 6-stage compiler pipeline and visualizes the output of every stage in real time. It is built as a **Compiler Construction** course project to make abstract compiler theory tangible and explorable.

**What you can do:**
- Type any SQL query and watch it flow through Lexer -> Parser -> Semantic -> Query Tree -> Optimizer -> Execution Plan
- Load 27 built-in sample queries covering valid queries, optimizer demos, and error cases at every phase
- Browse and edit the live database schema directly in the UI
- See interactive AST and Query Tree diagrams powered by Mermaid.js
- Observe error detection at the correct compiler phase (lexical, syntax, or semantic)

---

## System Architecture

```
+---------------------------------------------------------------------------+
|                        BROWSER  (Frontend UI)                             |
|                                                                           |
|  +-----------------------------------------------------------------------+|
|  |  index.html  +  css/style.css  +  js/app.js                          ||
|  |                                                                       ||
|  |  * SQL query editor with live syntax highlighting                     ||
|  |  * 27 built-in sample queries (valid, optimizer, error demos)         ||
|  |  * Live schema browser & editor (tables, columns, types, data)        ||
|  |  * 6-stage pipeline stepper with individual result panels             ||
|  |  * Mermaid.js -> interactive AST & Query Tree graph renders           ||
|  +-----------------------------------------------------------------------+|
|                             |  HTTP / REST API                            |
+-----------------------------+---------------------------------------------+
                              |
+-----------------------------v---------------------------------------------+
|                     FLASK SERVER  (backend/app.py)                        |
|                                                                           |
|  Static File Server  <--  serves frontend/                                |
|                                                                           |
|  REST Endpoints:                                                          |
|  * GET  /api/schema     <- load schema.json                               |
|  * POST /api/schema     <- save edited schema                             |
|  * GET  /api/samples    <- 27 built-in example queries                    |
|  * POST /api/compile    <- run compiler pipeline, return JSON phases      |
|                                                                           |
|  +-----------------------------------------------------------------------+|
|  |               COMPILER ENGINE (dual-mode)                             ||
|  |                                                                       ||
|  |  [1] C Binary (sql_compiler.exe)    <- preferred (if built)           ||
|  |      Flex lexer.l -> Bison parser.y -> ast.c -> semantic.c            ||
|  |      -> query_tree.c -> optimizer.c -> execution_plan.c               ||
|  |                                                                       ||
|  |  [2] Python Fallback (py_compiler.py) <- always available             ||
|  |      Full 6-stage pipeline in pure Python, zero extra deps            ||
|  +-----------------------------------------------------------------------+|
|                                                                           |
|  output/   <- JSON files written per compile request                     |
|  schema/   <- schema.json (database schema / catalog)                    |
+---------------------------------------------------------------------------+
```

---

## Compiler Pipeline -- Stage by Stage

```
SQL Query String
      |
      v
+-----------------------------------+
|  Stage 1 - LEXICAL ANALYSIS       |  lexer.l / py_compiler.py
|                                   |
|  Regex-based tokenizer            |
|  Token types:                     |
|   KEYWORD     (SELECT, FROM, ...) |
|   IDENTIFIER  (table / col names) |
|   INT_LITERAL / FLOAT_LITERAL     |
|   STRING_LITERAL  ('...')         |
|   BOOL_LITERAL    (TRUE / FALSE)  |
|   OPERATOR   (<= >= != = < >)     |
|   DELIMITER  (, ; * ( ) )         |
|  Tracks: line + column per token  |
|                                   |
|  Errors: illegal characters       |
|  Output -> output/tokens.json     |
+---------------+-------------------+
                |
                v
+-----------------------------------+
|  Stage 2 - SYNTAX ANALYSIS        |  parser.y / py_compiler.py
|                                   |
|  Grammar: SQL SELECT rules        |
|  Builds Abstract Syntax Tree      |
|                                   |
|  AST Node Types:                  |
|   SELECT_STMT  (root)             |
|   COLUMN_LIST / STAR              |
|   TABLE_NAME                      |
|   WHERE_CLAUSE                    |
|   BINARY_EXPR (AND / OR / rel-op) |
|   ORDER_BY_CLAUSE                 |
|   LIMIT_CLAUSE                    |
|                                   |
|  Errors: malformed grammar        |
|  Output -> output/ast.json        |
+---------------+-------------------+
                |
                v
+-----------------------------------+
|  Stage 3 - SEMANTIC ANALYSIS      |  semantic.c / py_compiler.py
|                                   |
|  Loads schema.json as catalog     |
|  Builds Symbol Table              |
|  Validates:                       |
|   [ok] Table exists in schema     |
|   [ok] All selected columns exist |
|   [ok] ORDER BY column exists     |
|  Warnings:                        |
|   [!] Duplicate SELECT columns    |
|   [!] Duplicate WHERE conditions  |
|                                   |
|  Output -> output/semantic.json   |
+---------------+-------------------+
                |
                v
+-----------------------------------+
|  Stage 4 - QUERY TREE             |  query_tree.c / py_compiler.py
|           (Relational Algebra)    |
|                                   |
|  Converts AST to RA operators:    |
|   SCAN             (table access) |
|   SELECTION  sigma (WHERE)        |
|   PROJECTION pi    (SELECT cols)  |
|   SORT       tau   (ORDER BY)     |
|   LIMIT      L                    |
|                                   |
|  Unoptimized bottom-up tree       |
|  Output -> output/query_tree.json |
+---------------+-------------------+
                |
                v
+-----------------------------------+
|  Stage 5 - QUERY OPTIMIZER        |  optimizer.c / py_compiler.py
|                                   |
|  Optimization passes:             |
|   Constant Folding                |
|     e.g. (1 = 1) -> TRUE          |
|   Boolean Simplification          |
|     e.g. TRUE AND cond -> cond    |
|   Duplicate Condition Removal     |
|   Selection Pushdown              |
|     (filter early, near scan)     |
|   Projection Pushdown             |
|     (restrict columns early)      |
|                                   |
|  Output -> output/optimized.json  |
+---------------+-------------------+
                |
                v
+-----------------------------------+
|  Stage 6 - EXECUTION PLAN         |  execution_plan.c / py_compiler.py
|            + BYTECODE             |
|                                   |
|  Physical Operators (ordered):    |
|   TABLE SCAN  cost O(N)           |
|   FILTER      cost O(N)           |
|   PROJECT     cost O(K)           |
|   ORDER BY    cost O(K log K)     |
|   LIMIT       cost O(1)           |
|                                   |
|  Pseudo-bytecode emitted:         |
|   SCAN   <table>                  |
|   FILTER <predicate>              |
|   PROJECT <columns>               |
|   SORT   <col> <ASC|DESC>         |
|   LIMIT  <n>                      |
|   RETURN                          |
|                                   |
|  Output -> output/execution_plan.json |
+-----------------------------------+
```

---

## Tech Stack

| Layer | Technology | Role |
|---|---|---|
| **Compiler (primary)** | ANSI C99, Flex, GNU Bison, GCC | Full pipeline compiled to `sql_compiler.exe` |
| **Compiler (fallback)** | Python 3 stdlib only | `py_compiler.py` -- same 6-stage pipeline, zero extra deps |
| **Backend API** | Python Flask 3.x | REST API server + static file host |
| **Frontend** | Vanilla HTML5 / CSS3 / JavaScript ES2020 | Query editor and result visualizer |
| **Graph Rendering** | Mermaid.js (CDN) | Interactive AST and Query Tree diagrams |
| **Schema Store** | JSON file (`schema/schema.json`) | Editable live database catalog |

---

## Folder Structure

```
sql-query-compiler-studio/
|
+-- backend/
|   +-- app.py                   # Flask REST API + static file server
|   +-- compiler/
|       +-- lexer.l              # Flex lexer grammar (SQL tokens)
|       +-- parser.y             # Bison parser grammar (SQL SELECT rules)
|       +-- main.c               # C binary entry point
|       +-- ast.c / ast.h        # AST node definitions and builder
|       +-- token.c / token.h    # Token type structures
|       +-- symbol_table.c/.h    # Symbol table (schema catalog)
|       +-- semantic.c/.h        # Semantic checker and validator
|       +-- query_tree.c/.h      # Relational algebra tree builder
|       +-- optimizer.c/.h       # Optimization passes
|       +-- execution_plan.c/.h  # Physical plan and bytecode emitter
|       +-- py_compiler.py       # Pure-Python fallback compiler (6 stages)
|       +-- test_compile.py      # Unit tests for Python compiler
|       +-- build.bat            # Windows build (Flex + Bison + GCC)
|       +-- Makefile             # Linux / macOS build
|
+-- frontend/
|   +-- index.html               # Single-page application shell
|   +-- css/
|   |   +-- style.css            # Dark theme, glassmorphism UI styles
|   +-- js/
|       +-- app.js               # All UI logic, API calls, Mermaid rendering
|
+-- schema/
|   +-- schema.json              # DB catalog: tables, columns, types, sample rows
|
+-- output/                      # Auto-generated JSON outputs per compile run
|   +-- query.sql
|   +-- tokens.json
|   +-- ast.json
|   +-- semantic.json
|   +-- query_tree.json
|   +-- optimized.json
|   +-- execution_plan.json
|
+-- package.json                 # npm scripts: "npm run dev" runs the server
+-- requirements.txt             # Python deps: flask>=3.0.0
+-- start.bat                    # Windows one-click launcher
+-- README.md
```

---

## Database Schema

The default `schema/schema.json` ships with **4 tables** and can be edited live via the Schema panel in the UI:

| Table | Columns | Sample Rows |
|---|---|---|
| `students` | `id`, `name`, `age`, `gpa`, `major`, `email`, `enrollment_year`, `is_active` | 10 |
| `employees` | `id`, `name`, `salary`, `department`, `position`, `hire_year`, `manager_id`, `is_active` | 10 |
| `courses` | `course_id`, `title`, `credits`, `department`, `instructor`, `max_seats`, `is_open` | 8 |
| `orders` | `order_id`, `customer_name`, `product`, `quantity`, `unit_price`, `status`, `order_year`, `is_paid` | 10 |

> Click **"Edit Schema"** in the UI to add or remove tables and columns at runtime. Changes are persisted to `schema.json` via the API.

---

## Quick Start

### Prerequisites

- **Python 3.8+** -- required (Flask server + fallback compiler)
- **Node.js / npm** -- optional (only for `npm run dev`)
- **MSYS2 + GCC + Flex + Bison** -- optional (only to build the C engine)

### Option 1 -- npm (recommended if Node.js is installed)

```powershell
npm run dev
```

### Option 2 -- Python directly

```powershell
python backend/app.py
```

### Option 3 -- Windows double-click

```
start.bat
```

Auto-installs Python deps, then starts the server.

### Option 4 -- Manual

```powershell
pip install -r requirements.txt
python backend/app.py
```

Then open your browser at **http://localhost:5000**

---

## Building the C Compiler Engine (Optional)

The project works out of the box via the Python fallback. To enable the faster ANSI C / Flex / Bison native engine:

**Windows (MSYS2 UCRT64)**

```bat
cd backend\compiler
build.bat
```

**Linux / macOS**

```bash
cd backend/compiler
make
```

Flask automatically detects `sql_compiler.exe` and prefers it over the Python fallback when present.

---

## REST API Reference

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/` | Serves `frontend/index.html` |
| `GET` | `/api/schema` | Returns current `schema.json` |
| `POST` | `/api/schema` | Persists updated schema |
| `GET` | `/api/samples` | Returns 27 built-in example queries |
| `POST` | `/api/compile` | Compiles SQL query; returns all 6 phase outputs |

**Compile Request**

```json
POST /api/compile
Content-Type: application/json

{
  "query": "SELECT name, age FROM students WHERE age >= 18 ORDER BY age ASC LIMIT 5;"
}
```

**Compile Response Shape**

```json
{
  "success": true,
  "engine": "Python SQL Compiler Engine",
  "tokens": [ { "type": "KEYWORD", "value": "SELECT", "line": 1, "column": 1 }, "..." ],
  "ast": { "type": "SELECT_STMT", "children": [ "..." ] },
  "semantic": {
    "success": true,
    "errors": [],
    "warnings": [],
    "symbol_table": { "table": "students", "columns": [ "..." ] }
  },
  "query_tree":   { "type": "LIMIT", "value": "5", "child": { "..." } },
  "optimized":    { "rules_applied": [ "..." ], "optimized_tree": { "..." } },
  "execution_plan": {
    "operators": [ { "step": 1, "op": "TABLE SCAN", "cost": "O(N)" }, "..." ],
    "bytecode":  [ "SCAN students", "FILTER age >= 18", "PROJECT name, age", "SORT age ASC", "LIMIT 5", "RETURN" ]
  }
}
```

---

## Built-in Sample Queries

27 queries are included, organized into four categories:

### Valid Queries -- Full Pipeline (1-12)

| # | Title | Table |
|---|---|---|
| 1 | Students - Filter by Age | `students` |
| 2 | Students - High GPA List | `students` |
| 3 | Students - Active Members | `students` |
| 4 | Students - Enrolled After 2022 | `students` |
| 5 | Employees - High Salary Filter | `employees` |
| 6 | Employees - Active Staff | `employees` |
| 7 | Employees - Senior Earners | `employees` |
| 8 | Courses - Open Courses | `courses` |
| 9 | Courses - High Credit Courses | `courses` |
| 10 | Orders - Paid Orders | `orders` |
| 11 | Orders - Large Quantity Orders | `orders` |
| 12 | Orders - Recent Year Filter | `orders` |

### Optimizer Demos (13-15)

| # | Title | What it demonstrates |
|---|---|---|
| 13 | Optimizer - Constant Folding (1=1) | `1 = 1 AND cond` collapsed to `cond` |
| 14 | Optimizer - Duplicate Column Warning | Same column selected twice triggers warning |
| 15 | Optimizer - Duplicate WHERE Condition | Redundant `AND` condition detected |

### Lexical Errors -- Stage 1 (16-18)

| # | Title | Error trigger |
|---|---|---|
| 16 | Lexical Error - Illegal Character (@) | `@` in column reference |
| 17 | Lexical Error - Hash Symbol in Query (#) | `#` used as operator |
| 18 | Lexical Error - Dollar Sign in Identifier | `$` prefix on column name |

### Syntax Errors -- Stage 2 (19-22)

| # | Title | Error trigger |
|---|---|---|
| 19 | Syntax Error - Missing FROM Clause | No `FROM` keyword |
| 20 | Syntax Error - Missing SELECT Keyword | Query starts with column name |
| 21 | Syntax Error - Incomplete ORDER BY | `ORDER` without `BY` |
| 22 | Syntax Error - No Column List | `SELECT FROM ...` |

### Semantic Errors -- Stage 3 (23-27)

| # | Title | Error trigger |
|---|---|---|
| 23 | Semantic Error - Column Not Found | Unknown column in SELECT |
| 24 | Semantic Error - Table Not Found | Non-existent table |
| 25 | Semantic Error - Invalid ORDER BY Column | Unknown column in ORDER BY |
| 26 | Semantic Error - Multiple Bad Columns | Several unknown columns |
| 27 | Semantic Error - Wrong Table for Column | Column from a different table |

---

## Output Files Reference

Each `/api/compile` call regenerates all files in `output/`:

| File | Contents |
|---|---|
| `query.sql` | Raw SQL query text written before compilation |
| `tokens.json` | Array of token objects: `type`, `value`, `line`, `column` |
| `ast.json` | Full Abstract Syntax Tree JSON |
| `semantic.json` | `success`, `errors[]`, `warnings[]`, `symbol_table` |
| `query_tree.json` | Unoptimized relational algebra tree |
| `optimized.json` | `rules_applied[]` + optimized tree |
| `execution_plan.json` | `operators[]` with cost estimates + `bytecode[]` |

---

## Running Tests

```powershell
python backend/compiler/test_compile.py
```

---

## License

MIT -- Built for educational purposes as a Compiler Construction course project.

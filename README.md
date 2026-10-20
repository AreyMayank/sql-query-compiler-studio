# SQL Query Compiler Studio

> An educational **Compiler Construction** project that demonstrates how modern relational database systems process SQL queries — from raw text to physical execution plans — through a full 6-stage compiler pipeline.

[![Python](https://img.shields.io/badge/Python-3.8%2B-blue?logo=python)](https://python.org)
[![Flask](https://img.shields.io/badge/Flask-3.x-black?logo=flask)](https://flask.palletsprojects.com)
[![C](https://img.shields.io/badge/C-ANSI--C99-grey?logo=c)](https://en.wikipedia.org/wiki/ANSI_C)
[![Flex](https://img.shields.io/badge/Flex-Lexer-orange)](https://github.com/westes/flex)
[![Bison](https://img.shields.io/badge/Bison-Parser-green)](https://www.gnu.org/software/bison/)
[![Mermaid.js](https://img.shields.io/badge/Mermaid.js-Diagrams-purple)](https://mermaid.js.org/)

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        BROWSER  (Frontend UI)                           │
│                                                                         │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │  index.html  +  css/style.css  +  js/app.js                       │ │
│  │                                                                    │ │
│  │  * CodeMirror-style query editor                                   │ │
│  │  * 6 built-in sample queries                                       │ │
│  │  * Live schema viewer & editor (tables, columns, types)            │ │
│  │  * Tabbed results: Tokens | AST | Semantic | Query Tree            │ │
│  │                    Optimizer | Execution Plan                      │ │
│  │  * Mermaid.js -> interactive AST & Query Tree graph renders        │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                             |  HTTP / REST API                          │
└─────────────────────────────┼───────────────────────────────────────────┘
                              |
┌─────────────────────────────v───────────────────────────────────────────┐
│                     FLASK SERVER  (backend/app.py)                      │
│                                                                         │
│  Static File Server  <--  serves frontend/                              │
│                                                                         │
│  REST Endpoints:                                                        │
│  * GET  /api/schema     <- load schema.json                             │
│  * POST /api/schema     <- save edited schema                           │
│  * GET  /api/samples    <- 6 built-in example queries                   │
│  * POST /api/compile    <- run compiler pipeline, return JSON phases    │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │               COMPILER ENGINE (dual-mode)                        │  │
│  │                                                                  │  │
│  │  [1] C Binary (sql_compiler.exe)    <- preferred (if built)      │  │
│  │      Flex lexer.l -> Bison parser.y -> ast.c -> semantic.c       │  │
│  │      -> query_tree.c -> optimizer.c -> execution_plan.c          │  │
│  │                                                                  │  │
│  │  [2] Python Fallback (py_compiler.py) <- always available        │  │
│  │      Full 6-stage pipeline in pure Python, zero extra deps       │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                         │
│  output/   <- JSON files written per compile request                   │
│  schema/   <- schema.json (database schema / catalog)                  │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Compiler Pipeline — Stage by Stage

```
SQL Query String
      |
      v
+-----------------------------------+
|  Stage 1 - LEXICAL ANALYSIS       |  lexer.l / py_compiler.py
|                                   |
|  Regex-based tokenizer            |
|  Token types:                     |
|   KEYWORD     (SELECT, FROM, ...)  |
|   IDENTIFIER  (table / col names) |
|   INT_LITERAL / FLOAT_LITERAL     |
|   STRING_LITERAL  ('...')         |
|   BOOL_LITERAL    (TRUE / FALSE)  |
|   OPERATOR   (<= >= != = < >)     |
|   DELIMITER  (, ; * ( ) )        |
|  Tracks: line + column per token  |
|                                   |
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
|   SELECTION  sigma (WHERE)        |
|   PROJECTION pi    (SELECT cols)  |
|   SORT       tau   (ORDER BY)     |
|   LIMIT      L                    |
|   SCAN             (table access) |
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
| **Compiler (fallback)** | Python 3 stdlib only | `py_compiler.py` — same 6-stage pipeline, zero extra deps |
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

The default `schema/schema.json` ships with **3 tables** and can be edited live via the Schema panel in the UI:

| Table | Columns | Column Types |
|---|---|---|
| `students` | `id`, `name`, `age`, `gpa`, `is_active` | INT, VARCHAR, INT, FLOAT, BOOLEAN |
| `employees` | `id`, `name`, `salary`, `department`, `is_active` | INT, VARCHAR, FLOAT, VARCHAR, BOOLEAN |
| `courses` | `course_id`, `title`, `credits` | INT, VARCHAR, INT |

> Click **"Schema"** in the UI sidebar to add/remove tables or columns at runtime — changes are persisted to `schema.json` via the API.

---

## Quick Start

### Prerequisites

- **Python 3.8+** — required (Flask server + fallback compiler)
- **Node.js / npm** — optional (only for `npm run dev`)
- **MSYS2 + GCC + Flex + Bison** — optional (only to build the C engine)

### Option 1 — npm (recommended if Node.js is installed)

```powershell
npm run dev
```

### Option 2 — Python directly

```powershell
python backend/app.py
```

### Option 3 — Windows double-click

```
start.bat
```

Auto-installs Python deps, then starts the server.

### Option 4 — Manual

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
| `GET` | `/api/samples` | Returns 6 built-in example queries |
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

| # | Title | What it demonstrates |
|---|---|---|
| 1 | Standard Query (Full Pipeline) | All 6 stages end-to-end |
| 2 | Constant Folding & Boolean Simplification | `1 = 1 AND cond` collapsed to `cond` |
| 3 | Duplicate Condition & Column Warning | Semantic warning detection |
| 4 | Semantic Error — Column Not Found | Error reporting on unknown column |
| 5 | Semantic Error — Table Not Found | Error reporting on unknown table |
| 6 | Employees Table Query | Multi-type schema, salary filter |

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

MIT — Built for educational purposes as a Compiler Construction course project.

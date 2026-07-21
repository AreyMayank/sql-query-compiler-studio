# Interactive SQL Query Compiler & Execution Plan Visualizer

An educational Compiler Construction project that demonstrates how modern database systems process SQL queries through compilers. This application compiles SQL `SELECT` queries through lexical, syntactic, semantic, and optimization phases to generate a physical execution plan and bytecode.

## 🚀 Compiler Pipeline Stages

1. **Lexical Analysis (Flex)**: Tokenizes inputs (Keywords, Identifiers, Numbers, Operators, Delimiters) and tracks line/column indices.
2. **Syntax Analysis (Bison)**: Parses query grammar rules and constructs an **Abstract Syntax Tree (AST)**.
3. **Semantic Analysis**: Validates Table/Column existence against the schema, ensures comparison operand type compatibility, and prints duplicates/ambiguities warnings.
4. **Relational Algebra Tree (Query Tree)**: Translates the AST into unoptimized relational algebra operators ($\sigma, \pi, \tau, L, \text{Scan}$).
5. **Query Optimizer**: Applies **Constant Folding**, **Boolean Simplification**, **Duplicate Condition Removal**, **Selection Pushdown**, and **Projection Pushdown**.
6. **Physical Execution Plan**: Creates execution pipelines (`TABLE SCAN` → `FILTER` → `PROJECT` → `ORDER BY` → `LIMIT`) and formats pseudo-bytecode.

---

## 🛠️ Tech Stack & Architecture

- **Compiler Engine**: ANSI C, Flex (Scanner), GNU Bison (Parser).
- **Backend API**: Python Flask.
- **Backend Fallback**: In-memory Python Compiler Engine (`py_compiler.py`) ensuring portability when MSYS2/GCC is not configured.
- **Frontend UI**: Vanilla JavaScript & CSS with dynamic **Mermaid.js** graph renderings.

---

## 📦 How to Run

### 1. Launch the API Server
Ensure Flask is installed:
```cmd
pip install flask
```
Run the application server:
```cmd
python backend/app.py
```
Open your browser and navigate to:
```
http://localhost:5000/
```

### 2. Build the C Compiler (Optional)
On Windows (using MSYS2 UCRT64/GCC):
```cmd
cd backend/compiler
build.bat
```

---

## 📂 Folder Structure

```
CompilerLab/
├── frontend/             # HTML, CSS, JS Web Visualizer
├── backend/              # Flask Backend API Server
│   └── compiler/         # C Lexer, Parser, AST, Semantic, Optimizer, ExecPlan
├── schema/               # schema.json defining tables and sample data
├── output/               # JSON outputs for compilation phases
└── README.md
```

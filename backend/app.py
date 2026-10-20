import os
import json
import subprocess
import sys
from flask import Flask, request, jsonify, send_from_directory

app = Flask(__name__, static_folder="../frontend", static_url_path="")

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(BASE_DIR)
OUTPUT_DIR = os.path.join(PROJECT_ROOT, "output")
SCHEMA_FILE = os.path.join(PROJECT_ROOT, "schema", "schema.json")
COMPILER_DIR = os.path.join(BASE_DIR, "compiler")
EXE_PATH = os.path.join(COMPILER_DIR, "sql_compiler.exe")
PY_COMPILER_PATH = os.path.join(COMPILER_DIR, "py_compiler.py")

os.makedirs(OUTPUT_DIR, exist_ok=True)

@app.route("/")
def serve_index():
    return send_from_directory(app.static_folder, "index.html")

@app.route("/<path:path>")
def serve_static(path):
    return send_from_directory(app.static_folder, path)

@app.route("/api/schema", methods=["GET"])
def get_schema():
    if os.path.exists(SCHEMA_FILE):
        with open(SCHEMA_FILE, "r") as f:
            return jsonify(json.load(f))
    return jsonify({"error": "Schema file not found"}), 404

@app.route("/api/schema", methods=["POST"])
def update_schema():
    new_schema = request.json
    with open(SCHEMA_FILE, "w") as f:
        json.dump(new_schema, f, indent=2)
    return jsonify({"success": True, "message": "Schema updated successfully"})

@app.route("/api/samples", methods=["GET"])
def get_samples():
    samples = [
        {
            "title": "1. Students — Full Pipeline",
            "query": "SELECT id, name, age, gpa FROM students WHERE age >= 18 ORDER BY gpa DESC LIMIT 5;"
        },
        {
            "title": "2. Students — Major & City Filter",
            "query": "SELECT name, major, city FROM students WHERE age > 20 ORDER BY name ASC LIMIT 10;"
        },
        {
            "title": "3. Constant Folding & Boolean Simplification",
            "query": "SELECT name, gpa FROM students WHERE 1 = 1 AND age > 20 ORDER BY gpa DESC LIMIT 5;"
        },
        {
            "title": "4. Duplicate Condition & Column Warning",
            "query": "SELECT name, name, age FROM students WHERE age > 18 AND age > 18;"
        },
        {
            "title": "5. Employees — High Salary Filter",
            "query": "SELECT name, salary, department, role FROM employees WHERE salary > 80000 ORDER BY salary DESC LIMIT 5;"
        },
        {
            "title": "6. Employees — By Experience",
            "query": "SELECT name, role, years_exp FROM employees WHERE years_exp >= 5 ORDER BY years_exp DESC LIMIT 8;"
        },
        {
            "title": "7. Courses — Credits Filter",
            "query": "SELECT title, credits, instructor FROM courses WHERE credits >= 4 ORDER BY title ASC LIMIT 10;"
        },
        {
            "title": "8. Orders — High Value Orders",
            "query": "SELECT customer, product, amount FROM orders WHERE amount > 100 ORDER BY amount DESC LIMIT 5;"
        },
        {
            "title": "9. Semantic Error (Column Not Found)",
            "query": "SELECT unknown_col, name FROM students WHERE age > 20;"
        },
        {
            "title": "10. Semantic Error (Table Not Found)",
            "query": "SELECT id, title FROM non_existing_table;"
        }
    ]
    return jsonify(samples)

@app.route("/api/compile", methods=["POST"])
def compile_query():
    data = request.json or {}
    query = data.get("query", "").strip()

    if not query:
        return jsonify({"success": False, "error": "Query cannot be empty"}), 400

    query_file_path = os.path.join(OUTPUT_DIR, "query.sql")
    with open(query_file_path, "w") as f:
        f.write(query)

    # Attempt running binary C compiler first if executable exists
    c_compiler_used = False
    if os.path.exists(EXE_PATH):
        try:
            res = subprocess.run([EXE_PATH, query_file_path, SCHEMA_FILE], cwd=PROJECT_ROOT, capture_output=True, text=True)
            c_compiler_used = True
        except Exception as e:
            c_compiler_used = False

    # If binary compiler isn't built yet or failed invocation, use Python Compiler Engine
    if not c_compiler_used:
        res = subprocess.run([sys.executable, PY_COMPILER_PATH, query_file_path, SCHEMA_FILE], cwd=PROJECT_ROOT, capture_output=True, text=True)

    # Read output files
    def load_json_output(filename):
        filepath = os.path.join(OUTPUT_DIR, filename)
        if os.path.exists(filepath):
            try:
                with open(filepath, "r") as f:
                    return json.load(f)
            except Exception:
                return None
        return None

    tokens = load_json_output("tokens.json")
    ast = load_json_output("ast.json")
    semantic = load_json_output("semantic.json")
    query_tree = load_json_output("query_tree.json")
    optimized = load_json_output("optimized.json")
    execution_plan = load_json_output("execution_plan.json")

    response_payload = {
        "success": semantic.get("success", False) if semantic else False,
        "engine": "MSYS2 Flex/Bison GCC Engine" if c_compiler_used else "Python SQL Compiler Engine",
        "tokens": tokens or [],
        "ast": ast or {},
        "semantic": semantic or {},
        "query_tree": query_tree or {},
        "optimized": optimized or {},
        "execution_plan": execution_plan or {}
    }

    return jsonify(response_payload)

if __name__ == "__main__":
    print("Starting SQL Compiler API Server on http://localhost:5000 ...")
    app.run(host="0.0.0.0", port=5000, debug=True)

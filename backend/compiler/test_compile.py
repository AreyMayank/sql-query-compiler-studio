import urllib.request
import json

def test_api():
    url = "http://localhost:5000/api/compile"
    query = "SELECT id, name, age FROM students WHERE 1 = 1 AND age >= 18 ORDER BY age ASC LIMIT 10;"
    data = json.dumps({"query": query}).encode('utf-8')

    req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'})
    try:
        with urllib.request.urlopen(req) as response:
            res = json.loads(response.read().decode('utf-8'))
            print("=== Compile API Call Status ===")
            print(f"Success: {res.get('success')}")
            print(f"Engine Used: {res.get('engine')}")
            
            print("\n=== Stage 1: Tokens ===")
            print(f"Tokens Count: {len(res.get('tokens', []))}")
            if res.get('tokens'):
                print(f"First Token: {res['tokens'][0]}")

            print("\n=== Stage 2: AST ===")
            print(f"AST Root Type: {res.get('ast', {}).get('type')}")

            print("\n=== Stage 3: Semantic ===")
            print(f"Semantic Check Success: {res.get('semantic', {}).get('success')}")
            print(f"Errors: {res.get('semantic', {}).get('errors')}")
            print(f"Warnings: {res.get('semantic', {}).get('warnings')}")

            print("\n=== Stage 4 & 5: Query Tree & Optimized Tree ===")
            print(f"Unoptimized Query Tree Type: {res.get('query_tree', {}).get('type')}")
            print(f"Optimizations Applied: {res.get('optimized', {}).get('rules_applied')}")

            print("\n=== Stage 6: Execution Plan & Bytecode ===")
            print(f"Bytecode Instructions:")
            for line in res.get('execution_plan', {}).get('bytecode', []):
                print(f"  {line}")

    except Exception as e:
        print("API test failed:", e)

if __name__ == '__main__':
    test_api()

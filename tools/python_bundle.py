#!/usr/bin/env python3
"""tools/python_bundle.py — Python-basert NCB bundlar for bootstrap-regen.

Brukar norscode_native til å kompilere kvar modul og omdøyper funksjonar.
Unngår bootstrap-sirkelen med bundler.no.

Bruk:
  python3 tools/python_bundle.py mod=fil.no mod2=fil2.no --output bundle.ncb.json
"""
import json, subprocess, sys, os

def compile_module(src_path: str, norscode_native: str) -> dict:
    """Kompiler ei .no-fil til NCB via norscode_native."""
    import tempfile
    with tempfile.NamedTemporaryFile(suffix=".ncb.json", delete=False) as f:
        out = f.name
    r = subprocess.run(
        [norscode_native],
        env={**os.environ, "NORSCODE_CMD": "compile",
             "NORSCODE_FILE": src_path, "NORSCODE_OUTPUT": out},
        capture_output=True, text=True
    )
    if r.returncode != 0:
        raise RuntimeError(f"Kompilering feilet for {src_path}: {r.stderr[:200]}")
    d = json.load(open(out))
    os.unlink(out)
    return d

def rename_functions(fns: dict, mod_name: str) -> dict:
    """Omdøyp __main__.X → mod_name.X"""
    result = {}
    for k, v in fns.items():
        new_k = k.replace("__main__.", mod_name + ".")
        result[new_k] = v
    return result

def main():
    args = sys.argv[1:]
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    native = os.path.join(root, "dist", "norscode_native")
    
    out_path = "bootstrap/kompiler.ncb.json"
    modules = []
    
    i = 0
    while i < len(args):
        if args[i] == "--output" and i + 1 < len(args):
            out_path = args[i + 1]
            i += 2
        elif "=" in args[i]:
            eq = args[i].index("=")
            mod_name = args[i][:eq]
            src_path = args[i][eq+1:]
            modules.append((mod_name, src_path))
            i += 1
        else:
            i += 1
    
    all_fns = {}
    for mod_name, src_path in modules:
        d = compile_module(src_path, native)
        fns = d.get("functions", {})
        renamed = rename_functions(fns, mod_name)
        all_fns.update(renamed)
        print(f"  [OK] {src_path} → {mod_name} ({len(renamed)} funcs)")
    
    bundle = {
        "format": "norscode-bytecode-v1",
        "entry": "__main__.start",
        "imports": [],
        "route_handlers": {},
        "dependency_providers": {},
        "guard_providers": {},
        "request_middlewares": [],
        "response_middlewares": [],
        "error_middlewares": [],
        "startup_hooks": [],
        "shutdown_hooks": [],
        "tests": {},
        "functions": all_fns
    }
    
    out = json.dumps(bundle, ensure_ascii=False)
    with open(out_path, "w") as f:
        f.write(out)
    print(f"Bundle: {out_path} ({len(out)} bytes, {len(all_fns)} funcs)")

if __name__ == "__main__":
    main()

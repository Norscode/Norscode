# REAL COMPILER V22

Nytt i v22:
- selfhost AST-broen støtter assignment til indeks
- compound assignment på indeks (`+=`, `-=`, `*=`, `/=`, `%=`)
- bytecode-backenden støtter `IndexSet`
- flate member-paths kan nå brukes som assignment-mål i AST-broen

Eksempel:
```bash
norcode selfhost-ast-export tests/test_selfhost_indexset_v22.no
norcode bytecode-run tests/test_selfhost_indexset_v22.shast.json --ast
```

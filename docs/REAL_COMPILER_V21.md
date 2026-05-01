# REAL COMPILER V21

## Nytt i v21

- `IfExpr` støttes nå som ekte uttrykksnode i AST-formatet `norscode-ast-v1`.
- `selfhost_ast_bridge` eksporterer ikke lenger `IfExpr` kun via statement-lowering.
- `bytecode_backend` kompilerer `IfExpr` direkte til labels og hopp.

## Ny test

```bash
norcode selfhost-ast-export tests/test_selfhost_ifexpr_v21.no
norcode bytecode-run tests/test_selfhost_ifexpr_v21.shast.json --ast
```

Forventet sluttverdi er `11`.

# REAL COMPILER V18

## Mål
Koble parser og bytecode-backend sammen via et eksplisitt AST-format.

## Nytt
- `compiler/ast_bridge.py`
- `norcode ast-export <fil.no>`
- `norcode bytecode-build <fil.nast.json> --ast`
- `norcode bytecode-run <fil.nast.json> --ast`

## Hvorfor
Dette gjør at en fremtidig selfhost-parser kan skrive `norscode-ast-v1` direkte, mens eksisterende bytecode-backend og VM kan brukes videre uten større omskriving.

## Verifisert i denne versjonen
- `.no -> .nast.json`
- `.nast.json -> .ncb.json`
- `.nast.json -> VM-run`

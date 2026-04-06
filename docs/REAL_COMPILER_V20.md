# REAL COMPILER V20

Denne versjonen utvider broen fra selfhost-parseren til `norscode-ast-v1`.

## Nytt i v20

- `ForEach` senkes til `While`-basert AST med eksplisitt indeks og `lengde(...)`
- compound assignment på navn senkes til vanlig `VarSet` + binæroperasjon
- `IfExpr` kan senkes i statement-kontekst:
  - `la x = hvis ...`
  - `returner hvis ...`
  - `x = hvis ...`
  - inline `if` brukt som expression-statement
- modul-kall støtter nå dypere modulsti som `a.b.c(...)`

## Verifisert eksempel

```bash
python3 main.py selfhost-ast-export tests/test_selfhost_bridge_v20.no
python3 main.py bytecode-build tests/test_selfhost_bridge_v20.shast.json --ast
python3 main.py bytecode-run tests/test_selfhost_bridge_v20.shast.json --ast
```

Forventet resultat:

```text
Return: 10
```

## Viktig status

Dette er fortsatt ikke full selfhost-kompilering, men v20 lukker flere av hullene mellom selfhost-parseren og bytecode-backenden.

Det som fortsatt gjenstår er blant annet:
- assignment til indeks/medlem
- full uttrykksnivå-støtte for `IfExpr` inni større uttrykk
- flere parity-kjøringer gjennom hele `.no -> .shast.json -> .ncb.json -> VM`-kjeden

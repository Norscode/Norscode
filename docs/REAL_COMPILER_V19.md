# REAL COMPILER V19

Denne versjonen legger til en første bro fra selfhost-parseren til AST-formatet `norscode-ast-v1`.

## Ny kommando

```bash
python3 main.py selfhost-ast-export tests/test_math.no
```

Dette skriver en `.shast.json`-fil som kan brukes av den eksisterende bytecode-backenden:

```bash
python3 main.py bytecode-build tests/test_math.shast.json --ast
python3 main.py bytecode-run tests/test_math.shast.json --ast
```

## Støttet foreløpig

- imports
- funksjoner
- `la`
- `returner`
- `hvis`
- `mens`
- `for ... = a til b`
- `bryt` / `fortsett`
- navn-assignment (`x = ...` eller `sett x = ...`)
- literals, lister, indeksering, kall og modul-kall

## Ikke støttet ennå

- `IfExpr`
- `ForEach`
- compound assignment (`+=` osv.)
- komplekse `Member`-uttrykk utenfor modul-kall

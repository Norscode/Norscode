# Real Compiler V26

V26 gjør selfhost-kjeden bedre for typede liste-deklarasjoner.

## Nytt
- `la t: liste_tekst = [""]` går nå riktig gjennom selfhost-kjeden
- selfhost AST-broen pakker enkeltverdier inn i `ListLiteral` når deklarert type er `liste_*`
- `selfhost-chain-check` inkluderer nå `tests/test_empty_string_list.no` i standardsettet

## Verifisering
```bash
python3 main.py selfhost-chain-run tests/test_empty_string_list.no
python3 main.py selfhost-chain-check
```

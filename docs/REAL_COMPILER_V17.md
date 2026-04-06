# REAL COMPILER V17

Dette steget innfører en første ekte `AST -> bytecode`-kjede i prosjektet.

## Hva som er nytt

- `compiler/bytecode_backend.py`
- `norcode bytecode-build <fil.no>`
- `norcode bytecode-run <fil.no>`
- `norcode bytecode-run <fil.ncb.json> --bytecode`

## Hva v17 støtter

- heltall, bool og tekst
- liste-literaler + `lengde(...)`
- variabler (`la`, assignment)
- `hvis`, `mens`, `for`
- funksjonskall i samme modul
- modulfunksjoner via import/alias
- builtins: `assert`, `assert_eq`, `assert_ne`, `skriv`, `tekst_fra_heltall`, `tekst_fra_bool`

## Verifisert i v17

Disse går grønt i bytecode-backenden:

- `tests/test_if.no`
- `tests/test_math.no`
- `tests/test_text.no`
- `tests/test_for.no`
- `tests/test_while.no`
- `tests/test_assert.no`
- `tests/test_assert_eq.no`
- `tests/test_dependency_import.no`
- `tests/test_empty_string_list.no`

## Status

Dette er en første bytecode-backend, ikke full erstatning for C-backenden eller full selfhost-backend ennå.

Det viktige i v17 er at kjeden nå finnes og virker:

`Norscode kilde -> AST -> bytecode (.ncb.json) -> VM`

## Neste steg

- koble selfhost-AST inn i samme backend
- utvide bytecode-dekning for flere AST-noder
- bygge native VM for `.ncb` som alternativ til Python-VM

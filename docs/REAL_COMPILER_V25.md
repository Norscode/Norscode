# REAL COMPILER V25

## Mål
Utvide standard selfhost-kjede til bredere testdekning.

## Nytt i v25
- `selfhost-chain-check` bruker nå et bredere standardsett
- flere eksisterende `.no`-tester går gjennom hele kjeden:
  `.no -> selfhost AST bundle -> bytecode -> VM`

## Verifisert
- `tests/test_if.no`
- `tests/test_math.no`
- `tests/test_text.no`
- `tests/test_dependency_import.no`
- `tests/test_assert.no`
- `tests/test_assert_eq.no`
- `tests/test_for.no`
- `tests/test_while.no`
- `tests/test_elif.no`
- `tests/test_selfhost_ifexpr_v21.no`
- `tests/test_selfhost_indexset_v22.no`

## Status
Dette er bredere selfhost-kjede-dekning, men ikke full komplett kjede ennå.
Kjente hull:
- `tests/test_empty_string_list.no`
- `tests/test_selfhost.no`

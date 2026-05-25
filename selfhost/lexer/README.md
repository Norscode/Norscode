# Norscode-native lexer

Dette området er startpunktet for lexer skrevet i Norscode.

Mål:

- Lage en lexer i Norscode som produserer samme token-kontrakt som dagens Python-lexer.
- Teste den mot eksisterende Python-lexer via parity fixtures.
- Bytte compiler frontend gradvis når token-parity er stabil.

## Kontrakt

Lexer skal produsere token-lister med disse feltene:

```json
{
  "type": "IDENT",
  "value": "start",
  "line": 1,
  "column": 10
}
```

## Første milepæl

M1 skal støtte:

- whitespace
- linjekommentarer
- navn/identifiers
- tall
- tekststrenger
- parenteser
- klammer
- komma
- kolon
- enkle operatorer
- norske nøkkelord

## Parity-regel

For hver kildefil skal Norscode-lexer og Python-lexer gi samme token-sekvens, bortsett fra eventuelle eksplisitt dokumenterte metadataforskjeller.

## Status

- [x] Definer token-format
- [x] Lag M1 lexer i Norscode
- [x] Lag token fixture-kommando
- [x] Lag parity-check mellom Python og Norscode lexer
- [ ] Koble lexer-service til valgfri Norscode lexer

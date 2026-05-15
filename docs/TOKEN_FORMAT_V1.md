# Norscode Token Format v1

Dette dokumentet definerer stabil token-kontrakt for Norscode.

Målet er at flere lexer-implementasjoner kan produsere samme tokenformat:

- dagens Python-lexer
- fremtidig Norscode-native lexer
- IDE-/tooling-lexer
- incremental lexer

## Token shape

```json
{
  "type": "IDENT",
  "value": "start",
  "line": 1,
  "column": 10
}
```

## Felt

- `type`: token-type som tekst
- `value`: token-verdi, kan være tekst, tall, bool eller null
- `line`: 1-basert linjenummer
- `column`: 1-basert kolonnenummer

## Krav

Alle token-streams skal ende med EOF-token.

```json
{
  "type": "EOF",
  "value": null,
  "line": 1,
  "column": 1
}
```

## Minimum token-kategorier for M1

M1-lexer skal støtte:

- EOF
- identifiers/navn
- keywords/nøkkelord
- numbers/tall
- strings/tekst
- parentheses
- braces
- brackets
- comma
- colon
- operators
- comments

## Parity-regler

1. Token-rekkefølgen må være lik mellom Python-lexer og Norscode-lexer.
2. `type`, `value`, `line` og `column` må være like.
3. Whitespace skal ikke eksporteres som token, med mindre en fremtidig versjon definerer det eksplisitt.
4. Kommentarer skal som hovedregel hoppes over hvis Python-lexer gjør det samme.
5. Breaking changes krever nytt formatnavn, for eksempel `norscode-token-v2`.

## Bruk i verktøy

Token-formatet brukes av:

- `lexer-parity-fixture`
- `lexer-parity-check`
- fremtidig Norscode-native lexer
- parser parity
- IDE/highlighting
- language server

## Neste steg

- Implementere M1 tokenisering i `selfhost/lexer/lexer_m1.no`
- Lage Norscode lexer-runner
- Sammenligne Norscode-tokenstream mot `.tokens.json`

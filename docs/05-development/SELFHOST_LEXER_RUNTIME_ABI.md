# Selfhost Lexer Runtime ABI

Dette dokumentet definerer runtime-kontrakten for å kjøre Norscode-native lexer fra verktøykjeden.

Målet er at vedlikehalds- og parity-verktøy mellombels kan kalle ein lexer skriven i Norscode gjennom VM/runtime, hente tokenlista og samanlikne henne mot historiske lexer-fixtures.

## Entry point

Selfhost lexer må eksponere:

```text
lex(kilde: tekst) -> liste
```

## Input

`kilde` er hele kildefilen som tekst.

## Output

Output skal være en liste med token-objekter som følger `docs/TOKEN_FORMAT_V1.md`.

```json
[
  {
    "type": "KEYWORD",
    "value": "funksjon",
    "line": 1,
    "column": 1
  },
  {
    "type": "EOF",
    "value": "",
    "line": 1,
    "column": 9
  }
]
```

## ABI-regler

1. Runtime må kunne laste `selfhost/lexer/lexer_m1.no`.
2. Runtime må kunne kalle funksjonen `lex` med én tekstparameter.
3. Runtime må returnere en liste med map/objekter.
4. Token-feltene må være `type`, `value`, `line`, `column`.
5. Feil skal returneres som strukturert runner-feil, ikke ukontrollert traceback.
6. ABI-en skal være stabil nok til å brukes av CI.

## Kommandoer som bruker ABI-en

- `selfhost-lexer-run`
- fremtidig `selfhost-lexer-parity`
- fremtidig `selfhost-lexer-suite`

## Fase 1 status

- [x] Lexer M1 workspace
- [x] Token format v1
- [x] historiske lexer-fixtures
- [x] Lexer parity suite
- [x] Selfhost lexer readiness
- [x] Runtime boundary
- [ ] VM function-call ABI
- [ ] Selfhost lexer token extraction
- [ ] Selfhost lexer vs historiske fixture-parity

## Neste steg

1. Lage en runtime call facade.
2. Eksponere `call_function(source_file, function_name, args)`.
3. Koble lexer-runneren til runtime call facade.
4. Sammenligne selfhost tokens mot `.tokens.json` fixtures.

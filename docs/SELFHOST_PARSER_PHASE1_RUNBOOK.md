# Selfhost Parser Phase 1 Runbook

Denne runbooken beskriver den minimale parser-gaten som nå brukes for Omgang 3 i
[`docs/SELFSTENDIG_NORSCODE_ROADMAP.md`](/Users/jansteinar/Projects/Norscode/docs/SELFSTENDIG_NORSCODE_ROADMAP.md).

## Mål

- verifisere at `selfhost/parser.no` kan lese tokenstrømmer fra den nye selfhost tokeniseren
- verifisere AST-bygging for funksjoner, uttrykk, precedence og deterministiske snapshot-utdata
- holde en liten, stabil parser-suite som kan kjøres uten å trekke inn Python-parsing

## Kjøring

Kjør parser-suiten:

```bash
./bin/nc selfhost-parser-suite
```

Kjør den direkte selfhost-testfila:

```bash
./bin/nc run selfhost/tests/parser_tests.no
```

## Hva som testes

- `returner`-setninger
- binære uttrykk og operatorpresedens
- nestede uttrykk
- funksjonsdeklarasjon med parametre, returtype og blokk
- deterministisk AST-output
- stabil AST-snapshot-streng

## Kontrakt

- parseren arbeider mot tokenlister, ikke rå tekst
- `selfhost/tests/parser_tests.no` bruker `selfhost/lexer/lexer_m1.no` som tokenkilde
- AST kan snapshottes via `selfhost.ast.ast_snapshot(...)`

## Forventet resultat

- `./bin/nc selfhost-parser-suite` skal rapportere `1/1 OK`
- `./bin/nc run selfhost/tests/parser_tests.no` skal avslutte uten feil

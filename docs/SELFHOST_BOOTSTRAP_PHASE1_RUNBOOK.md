# Selfhost Bootstrap Phase 1 Runbook

Denne runbooken beskriver bootstrap-gaten som nå brukes for Omgang 6 i
[`docs/SELFSTENDIG_NORSCODE_ROADMAP.md`](/Users/jansteinar/Projects/Norscode/docs/SELFSTENDIG_NORSCODE_ROADMAP.md).

## Mål

- verifisere at bootstrap-kjeden kan kjøres deterministisk i Norscode
- verifisere at output-hashen er stabil og kan sammenlignes mot golden snapshot
- halde ein enkel build/check-flyt utan skjulte fallback-reglar

## Kjøring

Kjør bootstrap-build:

```bash
./bin/nc selfhost-build
```

Kjør bootstrap-check:

```bash
./bin/nc selfhost-check
```

Kjør den direkte bootstrap-suiten:

```bash
./bin/nc run selfhost/tests/bootstrap_tests.no
```

## Hva som testes

- selfhost-kjeden fra lexer → parser → semantic → IR → bytecode
- deterministisk output-hash for en liten representativ funksjon
- golden-sammenligning mot [`selfhost/bootstrap.hash`](/Users/jansteinar/Projects/Norscode/selfhost/bootstrap.hash)
- at same input gir same output to gonger på rad

## Kontrakt

- `selfhost-build` rapporterer bootstrap-hash frå suiten
- `selfhost-check` køyrer suiten to gonger og samanliknar mot golden
- golden-fila held den stabile referansen for output-hashen

## Forventet resultat

- `./bin/nc selfhost-build` skal rapportere `OK`
- `./bin/nc selfhost-check` skal rapportere `OK`
- `./bin/nc run selfhost/tests/bootstrap_tests.no` skal avslutte uten feil

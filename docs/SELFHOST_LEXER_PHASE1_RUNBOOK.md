# Selfhost Lexer Phase 1 Runbook

Denne runbooken beskriver hvordan fase 1 for selfhost lexer skal kjøres og feilsøkes.

Målet med fase 1 er at `selfhost/lexer/lexer_m1.no` skal kunne:

1. bestå readiness
2. kompilere til bytecode
3. kjøres gjennom runtime ABI
4. returnere token-stream
5. matche Python lexer fixtures

## Kommandoer

### 1. Readiness

```bash
norcode-modular selfhost-lexer-status
```

Sjekker at `lexer_m1.no` inneholder nødvendige funksjoner og token-markører.

### 2. Compile smoke-test

```bash
norcode-modular selfhost-lexer-compile-check
```

Sjekker at selfhost lexeren kan kompileres gjennom `compiler_core`.

### 3. Python lexer fixtures

```bash
norcode-modular lexer-parity-suite --write
```

Lager eller oppdaterer `.tokens.json` fixtures fra dagens Python lexer.

### 4. Selfhost runtime run

```bash
norcode-modular selfhost-lexer-run tests/test_math.no
```

Prøver å kjøre `lex(kilde)` gjennom runtime ABI.

### 5. Selfhost parity

```bash
norcode-modular selfhost-lexer-parity tests/test_math.no
```

Sammenligner tokens fra selfhost lexer mot Python fixture.

### 6. Full suite

```bash
norcode-modular selfhost-lexer-suite --write-fixtures
```

Kjører hele QA-pipelinen.

### 7. Full suite uten runtime

```bash
norcode-modular selfhost-lexer-suite --write-fixtures --skip-runtime
```

Brukes når compile/fixture skal verifiseres før runtime ABI er helt stabil.

## Feilstadier

`selfhost-lexer-suite` klassifiserer feil i disse stadiene:

- `readiness`
- `compile`
- `fixture`
- `runtime`
- `parity`

## Typisk feilsøking

### Readiness-feil

Årsak:

- manglende funksjon i `lexer_m1.no`
- manglende token marker

Løsning:

- kjør `selfhost-lexer-status --json`
- legg til manglende funksjon eller token string

### Compile-feil

Årsak:

- `lexer_m1.no` bruker språkfeature som parser/compiler ikke støtter ennå
- typekontrakt passer ikke dagens semantic analyzer
- syntax er ikke kompatibel med Python parser

Løsning:

- kjør `selfhost-lexer-compile-check --json`
- forenkle syntax i `lexer_m1.no`
- unngå features som ikke støttes i bytecode-backend ennå

### Fixture-feil

Årsak:

- `.tokens.json` mangler
- Python lexer-output har endret seg

Løsning:

```bash
norcode-modular lexer-parity-suite --write
```

eller:

```bash
norcode-modular selfhost-lexer-suite --write-fixtures --skip-runtime
```

### Runtime-feil

Årsak:

- VM kan ikke kalle `lex`
- runtime ABI mangler støtte for argument/returverdi
- bytecode-backend mangler støtte for brukt språkfeature

Løsning:

- kjør `selfhost-lexer-run <fil> --json`
- sjekk `runtime_errors`
- sjekk at `lex(kilde)` finnes i bytecode-funksjonene

### Parity-feil

Årsak:

- selfhost lexer returnerer annen token-type
- verdi avviker
- line/column avviker
- EOF-kontrakt avviker

Løsning:

- kjør `selfhost-lexer-suite --json`
- se `first_diff`
- rett token-type, value, line eller column

## Akseptansekriterier for fase 1

Fase 1 regnes som fullført når:

```bash
norcode-modular selfhost-lexer-suite --write-fixtures
```

returnerer:

```text
Selfhost lexer suite: N/N OK
Readiness: OK
Compile: OK
Stages:
  ok: N
  readiness: 0
  compile: 0
  fixture: 0
  runtime: 0
  parity: 0
```

## Neste fase etter grønn fase 1

Når lexer-parity passerer, starter fase 2:

- Norscode-native parser workspace
- token-stream input fra selfhost lexer
- AST v1 builder
- parser parity fixtures
- parser runtime ABI

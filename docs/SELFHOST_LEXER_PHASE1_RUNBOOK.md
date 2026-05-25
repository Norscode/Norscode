# Selfhost Lexer Phase 1 Runbook

Denne runbooken beskriver hvordan fase 1 for selfhost lexer skal kjøres og feilsøkes.

Målet med fase 1 er at `selfhost/lexer/lexer_m1.no` skal kunne:

1. bestå readiness
2. kompilere til bytecode
3. validere at VM kan returnere TOKEN_FORMAT_V1-objekter
4. kjøres gjennom runtime ABI
5. returnere token-stream
6. matche Python lexer fixtures

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

### 3. Token object smoke-test

```bash
norcode-modular selfhost-lexer-token-smoke
```

Tester en minimal TOKEN_FORMAT_V1 dictionary-retur gjennom runtime ABI før hele lexeren kjøres.

Denne gaten isolerer:

- dictionary literals
- mixed token fields (`tekst` + `heltall`)
- VM return values
- runtime ABI object transport
- token validation

### 4. Python lexer fixtures

```bash
norcode-modular lexer-parity-suite --write
```

Lager eller oppdaterer `.tokens.json` fixtures fra dagens Python lexer.

### 5. Selfhost runtime run

```bash
norcode-modular selfhost-lexer-run tests/test_math.no
```

Prøver å kjøre `lex(kilde)` gjennom runtime ABI.

### 6. Selfhost parity

```bash
norcode-modular selfhost-lexer-parity tests/test_math.no
```

Sammenligner tokens fra selfhost lexer mot Python fixture.

### 7. Full suite

```bash
norcode-modular selfhost-lexer-suite --write-fixtures
```

Kjører hele QA-pipelinen inkludert token object smoke-test og den representative parity-pakken.

### 8. Full suite uten runtime

```bash
norcode-modular selfhost-lexer-suite --write-fixtures --skip-runtime
```

Brukes når compile/fixture skal verifiseres før runtime ABI er helt stabil.

Som standard kjører `selfhost-lexer-suite` disse representative filene:

- `tests/selfhost_lexer_token_smoke.no`
- `tests/selfhost_lexer_list_smoke.no`
- `tests/std.math.no`
- `tests/test_assert.no`
- `tests/test_assert_eq.no`
- `tests/test_assert_text.no`
- `tests/test_empty_int_list.no`
- `tests/test_empty_list_return.no`
- `tests/test_empty_string_list.no`
- `tests/test_empty_text_list.no`
- `examples/web_routes.no`
- `examples/web_openapi.no`
- `examples/web_request_response.no`
- `examples/web_methods.no`
- `examples/web_middleware.no`
- `examples/web_guard.no`
- `examples/web_openapi_auth.no`
- `examples/web_proxy.no`
- `examples/web_sanitize.no`
- `examples/web_validation.no`
- `examples/web_dependency.no`

### 9. Full suite uten token smoke

```bash
norcode-modular selfhost-lexer-suite --write-fixtures --skip-token-smoke
```

Brukes kun når token-smoke feiler kjent, men man fortsatt vil inspisere runtime/parity-diagnostikk.

## Feilstadier

`selfhost-lexer-suite` klassifiserer feil i disse stadiene:

- `readiness`
- `compile`
- `token_smoke`
- `fixture`
- `runtime`
- `validation`
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

### Token-smoke-feil

Årsak:

- VM kan ikke returnere dictionary/map-literal
- runtime ABI mister objektstruktur
- token-feltene følger ikke TOKEN_FORMAT_V1
- mixed field values (`tekst` og `heltall`) håndteres feil

Løsning:

```bash
norcode-modular selfhost-lexer-token-smoke --json
```

Se spesielt på:

- `runtime_ok`
- `token_object_ok`
- `validation_ok`
- `called_function`
- `available_functions`
- `candidate_functions`

Hvis denne feiler, fiks VM/object-return før full lexer-runtime feilsøkes.

### Fixture-feil

Årsak:

- `.tokens.json` mangler
- Python lexer-output har endret seg
- Python baseline-tokenene validerer ikke mot TOKEN_FORMAT_V1

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
- sjekk `available_functions`, `candidate_functions` og `called_function`
- sjekk at `lex(kilde)` finnes i bytecode-funksjonene

### Validation-feil

Årsak:

- selfhost lexer returnerer ikke en liste
- token mangler `type`, `value`, `line` eller `column`
- `line`/`column` er ikke gyldige heltall
- EOF-token mangler

Løsning:

- kjør `selfhost-lexer-run <fil> --json`
- se `validation_errors`
- rett TOKEN_FORMAT_V1-kontrakten i `lexer_m1.no`

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
Selfhost lexer suite: 21/21 OK
Readiness: OK
Compile: OK
Token smoke: OK
List smoke: OK
Stages:
  ok: N
  readiness: 0
  compile: 0
  token_smoke: 0
  list_smoke: 0
  fixture: 0
  runtime: 0
  validation: 0
  parity: 0
```

## Neste fase etter grønn fase 1

Når lexer-parity passerer, starter fase 2:

- Norscode-native parser workspace
- token-stream input fra selfhost lexer
- AST v1 builder
- parser parity fixtures
- parser runtime ABI

# Fase 9 M2 — Try/Catch M1

Mål: legge inn første stabile selfhost-støtte for kontrollert feilflyt med `prøv`, `fang` og `kast`.

## Syntax

```norscode
prøv
    kast "feil"
fang feil
    skriv(feil)
slutt
```

## Lexer

Må støtte token:

- `PROV` / `PRØV`
- `FANG`
- `KAST`
- `SLUTT`

## AST

Nye node-typer:

```text
TryCatch
  Blokk
  Catch(feil)
    Blokk

Throw
  Expr
```

## Parser

Parser må kunne lese:

- `prøv`-blokk
- `fang <navn>`-blokk
- `kast <uttrykk>` som setning
- nested try/catch

## Semantic

Regler:

- `kast` må ha et uttrykk
- `fang` må definere catch-variable i eget scope
- catch-variable skal ha type `Error` i M1
- try-blokk og catch-blokk analyseres separat
- nested try/catch skal være lov

## IR/backend

Nye IR-ops:

```text
TRY_BEGIN <catch_label>
TRY_END
CATCH <navn>
THROW
RETHROW
```

For:

```norscode
prøv
    risky()
fang feil
    skriv(feil)
slutt
```

Forventet senking:

```text
TRY_BEGIN catch_0
CALL risky
TRY_END
JUMP try_end_1
LABEL catch_0
CATCH feil
LOAD feil
CALL skriv
LABEL try_end_1
```

## Runtime

Runtime må senere støtte:

- exception stack
- stack unwind
- rethrow
- stack trace
- nested handlers

## Exit-kriterier

Try/Catch M1 er ferdig når:

- AST kjenner `TryCatch`, `Catch` og `Throw`
- parser kan lese M1-syntax
- semantic lager catch-scope
- backend lager try/catch IR
- parity fixture gir deterministisk AST/IR/hash

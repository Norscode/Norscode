# Selfhost boolean/operator stabilization plan

## Bakgrunn

Nylige parity-feil viser avvik mellom Python-backend og selfhost-backend rundt:

- `NOT`
- `AND`
- `OR`
- operatorrekkefølge
- token-normalisering for engelske uttrykk (`less than`, `less than or equal`)
- parser-state rundt nested uttrykk

Eksempel på observert mismatch:

```text
Forventet:
0: PUSH 1
1: PUSH 0
2: NOT
3: AND
4: PUSH 0
5: OR
6: PRINT
7: HALT

Fikk:
0: PUSH 1
1: PUSH 0
2: SWAP
3: NOT
4: SWAP
5: OR
6: PRINT
7: HALT
```

Dette peker mot:

1. feil precedence
2. feil stack-rekkefølge
3. manglende eksplisitt AND-emisjon
4. mulig fallback-path i selfhost compiler

---

## Prioriterte tiltak

### Fase 1 — Parser parity

- Stabiliser boolske operatorer
- Verifiser precedence-tabell
- Fjern implicit SWAP der operatoren er binær
- Sikre identisk AST mellom Python og selfhost

Mål:

- `test_selfhost.no` skal være grønn
- parity fixtures skal være identiske

---

### Fase 2 — IR normalisering

Legg til normaliseringspass:

- redundant SWAP-fjerning
- constant folding
- boolsk operator-normalisering
- dead-stack cleanup

Mål:

- identisk IR for selfhost og bootstrap
- mindre bytecode
- enklere debugging

---

### Fase 3 — Selfhost readiness gate

Utvid CI med:

```bash
norcode selfhost-parity --suite all
norcode selfhost-parity-progress --require-ready
```

Legg til egen pipeline:

- bootstrap compiler
- selfhost compile
- selfhost compile selfhost compiler
- bytecode diff
- runtime diff

---

## Anbefalt implementasjonsrekkefølge

1. tokenizer
2. precedence parser
3. AST parity
4. bytecode backend
5. IR optimizer
6. CI gate
7. selfhost bootstrap chain

---

## Kritiske mål

### Minimum

- full parity for boolske uttrykk
- stabil parser
- deterministisk bytecode

### Neste nivå

- full selfhost compiler
- compiler compiler parity
- native backend uten Python-runtime

### Langsiktig

- full Norscode toolchain i Norscode
- frontend/runtime/server stack
- package ecosystem
- binary-first distribusjon

---

## Status

Repository har allerede:

- moden CLI
- package manager
- registry/cache
- web runtime
- OpenAPI-generator
- CI-system
- selfhost parity tooling
- release tooling
- binary bootstrap-flyt

Neste kritiske milepæl er:

> 100% stabil selfhost parity.

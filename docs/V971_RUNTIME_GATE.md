# v971-v980 runtime gate

## Current status on 2026-06-27

The runtime gate is now green in the AIKernel production matrix:

```text
prosjekter/NorscodeAIKernel/status/production_readiness.md
```

The no-status below is retained as historical context.

Dette dokumentet definerer blokkeringane før produksjonsmerking.

## Må vere grønt

- `builtin.random_hex(n)` med sikker OS-random.
- `builtin.random_bytes(n)` eller tilsvarande intern bytekjelde.
- `builtin.uuid()` dersom UUID blir eksponert.
- Stabil `tid`, `now` og `timestamp`.
- `exec_prosess` med policy eller deaktivert i produksjon.

## Historical no-status

```json
{
  "production_ready": false,
  "reason": "stage0_mangler_random_hex"
}
```

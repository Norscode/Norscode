# Ventar på seed-aktivering (Fase 3) — validert via `nc run-pure`

Testar her importerer nye `selfhost.*`-modular som ENNO ikkje er i den committa
stage-0-seeden sin innebygde bunt. Via vanleg `nc run`/`nc test` (committa seed)
gjev `bruk selfhost.<ny>` «Ukjent funksjon» (seed-aktiveringsgapet) — modulen
aktiverer seg først når seeden blir rebygd av sjølvhosta codegen (roadmap Fase 3).
Difor er dei flytta ut av `tests/` (som `nc test` oppdagar med `-maxdepth 1`) slik
at suita held seg ærleg grøn.

## Slik validerer du dei NO (soft-seed-aktivering)

`nc run-pure <fil.no>` bundlar fila + alle transitive `selfhost.*`/`std.*`-modular
frå arbeidstreet (via hybrid-bundlaren) og køyrer i den reine VM-en — nye modular
blir dermed aktive utan native seed-rebygg. Program-stdout og returverdi (→ exit-
kode) blir propagert, så ein test som returnerer 42 (OK) / 1 (FEIL) kan verifiserast:

```
./bin/nc run-pure tests/pending_seed_activation/test_ncb_serde_dualformat.no
# → "test_ncb_serde_dualformat: OK (303 binærbyte, deterministisk)", exit 42
```

## Testar

- `test_ncb_serde_dualformat.no` — binær/JSON dual-format NCB-serde
  (`selfhost.ncb_serde`, `selfhost.ncb_bin`). **Validert grøn via `nc run-pure`
  (exit 42)** etter null-decode-fiksen (`fc6e681`). Flytt tilbake til `tests/` når
  Fase 3-seeden aktiverer modulane i `nc test`.

# Ventar på seed-aktivering (Fase 3)

Testar her importerer nye `selfhost.*`-modular som ENNO ikkje er i den committa
stage-0-seeden sin innebygde bunt. På den committa seeden gjev `bruk selfhost.<ny>`
«Ukjent funksjon» (seed-aktiveringsgapet) — modulen aktiverer seg først når seeden
blir rebygd av sjølvhosta codegen (roadmap Fase 3).

Difor er dei flytta ut av `tests/` (som `nc test` oppdagar med `-maxdepth 1`) slik at
suita held seg ærleg grøn. Flytt dei tilbake når Fase 3-seeden aktiverer modulane.

- `test_ncb_serde_dualformat.no` — binær/JSON dual-format NCB-serde (`selfhost.ncb_serde`,
  `selfhost.ncb_bin`). Biblioteket er landa (inert infra); testen ventar på aktivering.

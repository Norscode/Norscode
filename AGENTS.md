## Imported project instructions

### Selvstendighet (obligatorisk for agenter)

Norscode skal være **selvstendig uten Python og C** i aktiv utvikling, bygg, release og CI.

- **Normal kjede:** `.no` → lexer/parser/semantic/bytecode → **NCB JSON** → `selfhost/vm.no` (og `selfhost/json.no` for parsing).
- **CLI:** `./bin/nc` og `dist/norscode_native` (stage-0-binær); nye funksjonar skal sjekkast med `./bin/nc feature-check [fil.no ...]`.
- **Ikke legg til:** Python-verktøy, C-verktøy, NCBB-konvertering, nye C-VM-steg i CI, eller `ncb_to_c` i produksjonsflyt.
- **Historikk:** Eldre C/Python-spor skal berre liggje under `archive/`; ikkje legg `.py`, `.c` eller `.h` i aktiv `tools/`, `selfhost/`, `bin/`, `bootstrap/` eller CI-flate.
- **Dokumentasjon:** [docs/SELFHOST_HANDLINGSPLAN.md](docs/SELFHOST_HANDLINGSPLAN.md)

Ved tvil: kortare normal vei i Norscode, tydeligere legacy-markering.

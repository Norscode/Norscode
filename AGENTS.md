## Imported project instructions

### Selvstendighet (obligatorisk for agenter)

Norscode skal være **selvstendig uten Python og C** i normal utvikling, bygg og CI.

- **Normal kjede:** `.no` → lexer/parser/semantic/bytecode → **NCB JSON** → `selfhost/vm.no` (og `selfhost/json.no` for parsing).
- **CLI:** `./bin/nc` og `dist/norscode_native` (stage-0-binær); ikke introduser `tools/*.py` for kompilering eller kjøring.
- **Ikke legg til:** NCBB-konvertering i Python, nye C-VM-steg i CI, eller `ncb_to_c` i produksjonsflyt.
- **Legacy:** Eldre C-VM (`archive/c_minimal_vm/`) og Python kun under `scripts/` (f.eks. `gen_expr_fraser.py`) — ikkje legg `.py` i `tools/`.
- **Dokumentasjon:** [docs/SELFHOST_HANDLINGSPLAN.md](docs/SELFHOST_HANDLINGSPLAN.md)

Ved tvil: kortare normal vei i Norscode, tydeligere legacy-markering.

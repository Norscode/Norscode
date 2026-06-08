# Plan: full selvstendighet for Norscode

Operativ plan som bygger på [SELFHOST_HANDLINGSPLAN.md](SELFHOST_HANDLINGSPLAN.md).
Målet er at **normal utvikling, CI og release** ikkje treng Python, og at **stage-0** kan fornyast frå Norscode sjølv (med ein engangs clang-seed).

## Definisjon: «helselvstendig»

| Nivå | Kriterium | Status |
|------|-----------|--------|
| **L1** | Ingen Python i `tools/`, CI eller CLI | ✅ |
| **L2** | `./bin/nc run/test/compile` via `norscode_native` (NORSCODE_CMD) | ✅ |
| **L3** | Selfhost-gate + bootstrap-self (steg A–C) i CI | ✅ |
| **L4** | Regenerer `bootstrap/maint/c/` frå `.no` utan Python (`tools/maint/regen_native.sh`) | ✅ verktøy (vedlikehald) |
| **L5** | Kompilator-bundle deterministisk (Gen1 == Gen2, `tools/selfcompile_l5.sh`) | ✅ |
| **L5b** | Gen1-NCB `bygg_bundle`-bytekode produserer identisk Gen2 (`tools/selfcompile_l5b.sh`) | ✅ |
| **L6** | Ingen føregenerert `bootstrap/maint/c/` i repo (seed → regen → clang) | ✅ |

**Stage-0:** Seed frå `bootstrap/stage0/` eller GitHub Release → `tools/maint/regen_native.sh` → `bootstrap/maint/c/` → clang. Dette er berre maintainer-/bootstrap-lane, ikkje normal utviklingskjede. Historisk C-host ligg i `archive/legacy_c_backend/`, ikkje i aktiv normalflate.

## Kjede i dag

```text
seed (stage0 / release)  →  tools/maint/regen_native.sh  →  bootstrap/maint/c/*.c
        ↑                                              ↓
        └──────── dist/norscode_native  ←──────── clang
.no → bundle → kompiler.ncb.json → archive/legacy_c_backend/ncb_to_c.no + gen_dispatch   (L4/vedlikehald, utan Python)
```

```text
Dagleg bruk:
  program.no  →  nc (native)  →  NCB JSON  →  VM / native køring
```

Alt som går via `bootstrap/maint/c/`, `archive/legacy_c_backend/` eller clang er vedlikehald eller historikk, ikkje normal brukarveg.

## Omganger

### Omgang A — Verifiser og lås L1–L3 (no)

**Leveranser**

- `tools/verify_selvstendighet.sh` — éin kommando for gate + bootstrap-self + test
- `tools/maint/regen_native.sh` — regenerer `bootstrap/maint/c/` frå seed (utan Python, maintainer-only)
- CI: `./bin/nc bootstrap-self` på macOS og Linux
- CI: `bash tools/verify_selvstendighet.sh` på Ubuntu

**Ferdig når:** Grønn CI utan manuell release-binær.

### Omgang B — Regen i CI (valgfri / manuell)

**Leveranser**

- `tools/maint/regen_verify.sh` — regenerer til `build/regen_verify/`, samanlikn SHA-256 med `bootstrap/`
- `.github/workflows/regen_bootstrap.yml` — `workflow_dispatch` + PR på `selfhost/**`
- `REGEN_ROOT` på `tools/maint/regen_native.sh` — regen utan å overskrive repo

**Ferdig når:** Maintainer kan oppdatere stage-0 med éin kommando etter kompilator-endring.

**Status:** ✅ verktøy, CI og grønn `regen_verify` i vedlikehaldsløypa (krev at `bootstrap/` er regenert med same native som CI byggjer frå `bootstrap/maint/c/`).

### Omgang C — Sjølvkompilering (L5)

**Leveranser (L5, implementert)**

- `tools/selfcompile_l5.sh` / `./bin/nc selfcompile-l5`
- To full bundle-kjøringar med same `norscode_native` → `build/l5/compiler_v1.ncb.json` og `v2`
- Byte-paritet (cmp) mellom generasjonar
- Inkludert i `tools/verify_selvstendighet.sh`

**L5b (framtid):** Gen1-NCB køyrer `selfhost.bundler.bygg_bundle` via VM; krev at tolker og bootstrap-host-dispatch er ekvivalente.

**Ferdig når:** `compiler_v1.ncb` og `compiler_v2.ncb` er identiske (✅ for L5).

### Omgang D — Fjern stage-0-C frå repo (L6)

**Leveranser**

- `bootstrap/maint/c/` generert lokalt (`bootstrap/maint/c/README.md`), ikkje i git
- `tools/build_norscode_native.sh` — seed som default; `REGEN=1` for regen → clang
- `tools/maint/verify_l6.sh` / `tools/maint/regen_verify.sh` — deterministisk regen-gate
- Seed: `bootstrap/stage0/` eller GitHub Release

**Ferdig når:** Ingen committed `norscode_generated.c` (✅).

### Omgang E — Legacy-utfasing

**Leveranser**

- `archive/c_minimal_vm/README.md` — historisk C-VM (fysisk fjerna frå `tools/` i Omgang 0)
- `archive/legacy_c_backend/ncb_to_c.no` / `selfhost/maint/gen_dispatch.no` — berre for regen, ikkje brukar-CLI
- README, SELFHOST_STATUS, ARCHIVE_INDEX oppdatert

**Status:** ✅ dokumentert; `tools/c_minimal_vm/` er fjerna; gate `tools/no_legacy_cvm.sh`.

## Kommandoar (etter omgang A)

```bash
bash tools/build_norscode_native.sh      # stage-0 (L6)
bash tools/verify_selvstendighet.sh       # L1–L6 + native testløp
bash tools/selfcompile_l5.sh             # L5 byte-paritet
./bin/nc selfcompile-l5b                 # L5b Gen1-bytekode → Gen2
bash tools/maint/regen_native.sh        # maintainer: generer bootstrap/maint/c/ (krev seed)
bash tools/maint/regen_verify.sh        # deterministisk regen (to kjøringar)
bash tools/maint/verify_l6.sh           # L6-gate (git + regen)
./bin/nc selfhost-bootstrap-gate
./bin/nc bootstrap-self
```

## Sannhetsregel

Kortare veg mellom `.no` og det brukaren køyrer = meir selvstendig.
Kvar ny Python- eller NCBB-avhengighet i normal flyt er eit steg tilbake.

# Plan: full selvstendighet for Norscode

Operativ plan som bygger på [SELFHOST_HANDLINGSPLAN.md](SELFHOST_HANDLINGSPLAN.md).
Målet er at **normal utvikling, CI og release** ikkje treng Python, og at **stage-0** kan fornyast frå Norscode sjølv (med ein engangs clang-seed).

## Definisjon: «helselvstendig»

| Nivå | Kriterium | Status |
|------|-----------|--------|
| **L1** | Ingen Python i `tools/`, CI eller CLI | ✅ |
| **L2** | `./bin/nc run/test/compile` via `norscode_native` (NORSCODE_CMD) | ✅ |
| **L3** | Selfhost-gate + bootstrap-self (steg A–C) i CI | 🔄 denne planen |
| **L4** | Regenerer `bootstrap/c/` frå `.no` utan Python (`tools/regen_native.sh`) | 🔄 denne planen |
| **L5** | Kompilator-kompilerer-kompilator med identisk NCB (byte-paritet) | ⬜ framtid |
| **L6** | Ingen føregenerert `bootstrap/c/` i repo (berre `.no` + seed) | ⬜ framtid |

**Stage-0-unntak (inntil L6):** Éin `norscode_native` per plattform byggast med clang frå `bootstrap/c/`, eller hentast frå release.

## Kjede i dag

```text
bootstrap/c/*.c  +  clang  →  dist/norscode_native   (stage-0)
        ↑
   sjekka inn / regen
        ↑
.no → bundle → kompiler.ncb.json → ncb_to_c + gen_dispatch   (L4, utan Python)
        ↑
dist/norscode_native (seed)
```

```text
Dagleg bruk:
  program.no  →  nc (native)  →  NCB JSON  →  VM / native køring
```

## Omganger

### Omgang A — Verifiser og lås L1–L3 (no)

**Leveranser**

- `tools/verify_selvstendighet.sh` — éin kommando for gate + bootstrap-self + test
- `tools/regen_native.sh` — regenerer `bootstrap/c/` frå seed (utan Python)
- CI: `./bin/nc bootstrap-self` på macOS og Linux
- CI: `bash tools/verify_selvstendighet.sh` på Ubuntu

**Ferdig når:** Grønn CI utan manuell release-binær.

### Omgang B — Regen i CI (valgfri / manuell)

**Leveranser**

- `workflow_dispatch`: regenerer og samanlikn hash av `norscode_generated.c`
- Dokumenter når commit av ny `bootstrap/c/` er naudsynt

**Ferdig når:** Maintainer kan oppdatere stage-0 med éin kommando etter kompilator-endring.

### Omgang C — Sjølvkompilering (L5)

**Leveranser**

- `bin/nc` kompilerer `selfhost/kompiler.no` → NCB
- Køyr same NCB via `selfhost/vm.no` eller innebygd executor
- Byte-paritet mellom generasjonar (som `bootstrap_gate` steg 3, utvid)

**Ferdig når:** `compiler_v1.ncb` og `compiler_v2.ncb` er identiske.

### Omgang D — Fjern stage-0-C frå repo (L6)

**Leveranser**

- Minimal seed-binær i release / `bootstrap/stage0/` berre
- CI: seed → regen → clang → test (ingen committed `norscode_generated.c`)

**Ferdig når:** Repo-storleik og «kvifor ligg det 1 MB C her?» er borte.

### Omgang E — Legacy-utfasing

**Leveranser**

- `tools/c_minimal_vm/` → `archive/` eller slett
- `selfhost/ncb_to_c.no` berre for regen, ikkje dokumentert som brukervei
- Oppdater README, ROADMAP, SELFHOST_STATUS

## Kommandoar (etter omgang A)

```bash
bash tools/build_norscode_native.sh      # stage-0
bash tools/verify_selvstendighet.sh       # gate + steg C + 48 tester
bash tools/regen_native.sh               # forny bootstrap/c/ (treg, krev seed)
./bin/nc selfhost-bootstrap-gate
./bin/nc bootstrap-self
```

## Sannhetsregel

Kortare veg mellom `.no` og det brukaren køyrer = meir selvstendig.
Kvar ny Python- eller NCBB-avhengighet i normal flyt er eit steg tilbake.

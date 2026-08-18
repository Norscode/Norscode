# JSON-avvikling — plan for å fjerne dei 319 committede `.ncb.json`

> Mål: **null committa JSON-bytekode i repoet.** Trust-anchor skal vere dei native
> seed-binærane åleine (`bootstrap/stage0/norscode-<plattform>`). All bytekode
> (kompilator, stdlib, app-bundlar) blir regenerert *natively* frå `.no`-kjelde —
> anten embedda i seeden eller materialisert til ein git-ignorert build-cache.

## 1. Kva dei 319 filene faktisk er

| Gruppe | ~Tal | Rolle | Vanskegrad å fjerne |
|---|---|---|---|
| `bootstrap/precompiled/*.ncb.json` | 26 | Kompilatoren sin eigen bytekode (parser, lexer, semantic, vm, nc_main, std …) — **trust-anchor** | Høg (kjerne) |
| `bootstrap/precompiled_fragments{,_inner}/*.json` | 16 | Fragment-bundlar for inkrementell self-compile | Høg |
| `bootstrap/stdlib/*.ncb.json` | 39 | Stdlib som bytekode, lasta i runtime | Middels |
| `bootstrap/kompiler.ncb.json` | 1 | Entry-bundle | Middels |
| `examples*/**.ncb.json`, `tests*/**payloads`, `startproject_templates/**` | ~200 | Per-app bytekode-**cache** / testfixture | Låg (regenererbar) |
| `Hjemmeside/**.ncb.json`, `selfhost/*.ncb.json`, div. | ~30 | App-bytekode + verktøy-cache | Låg |

Kjerneutfordringa er dei ~80 filene i `bootstrap/`. Resten (~240) er cache/fixture
som seeden kan regenerere trivielt når kjerna er på plass.

## 2. Kvifor JSON-en finst i dag (låsingane)

1. **Runtime-lasting**: `selfhost/vm.no`, `selfhost/bundler.no`,
   `selfhost/elf_compile_driver.no` les `bootstrap/precompiled/*` for å sleppe å
   re-parse `.no` kvar køyring (fart + å ha ein kjend-god kompilator).
2. **Self-sufficiency-gaten**: `selvstendighet.yml` / `tools/verify_selvstendighet.no`
   regenererer bytekode frå kjelde og samanliknar **byte-for-byte mot den committede
   JSON-en**. JSON-en er altså *fasiten* i paritetssjekken.
3. **Bootstrap-tillit**: precompiled-JSON er det ferske laget over den (eldre)
   embedda bytekoden i stage0-binæren; fleire `tools/build_*_candidate.no` byggjer
   nye artefaktar *frå* denne JSON-en.

Alle tre må flyttast frå «JSON på disk» til «native seed» før filene kan slettast.

## 3. Målarkitektur

```
FØR:  .no kjelde ──parse/kompiler──▶ bootstrap/precompiled/*.ncb.json (COMMITTA)
                                          │ lasta i runtime
                                          ▼
                                    stage0-binær (eldre embedded bytekode)

ETTER: .no kjelde ──native seed (kompiler + stdlib EMBEDDA i binæren)──▶ bytekode
                                          │ materialisert til
                                          ▼
                                    build/cache/ (GIT-IGNORERT, aldri committa)
```

- Seeden ber sin **eigen** kompilator- og stdlib-bytekode internt (TEXT_V1-embed i
  ELF/Mach-O/PE). Ingen ekstern `bootstrap/precompiled/*` trengst for at seeden skal
  kunne kompilere.
- Runtime som i dag les precompiled-JSON, les i staden frå **embedded** (når vi
  køyrer seeden) eller frå ein **regenerert build-cache** (når vi byggjer frå kjelde
  med ein interpreter).
- Paritet blir **reproduserbarheit**: regenerer to gonger frå kjelde → identisk
  output (i staden for å samanlikne mot committa fasit).

## 4. Fasar

### Fase 0 — Reelt frie slettingar (verifisert 2026-08-18)
**KORRIGERT:** Det meste av det eg først kalla «daudt» er framleis lastberande:
- `archive/legacy_c_backend/*.c` er **live ABI-fasit** — `tools/release_preflight.no`
  les innhald frå `nc_native_main.c`/`nc_windows_backend.c`/`nc_runtime_mini.c` som
  ~20 assertar — **og** live byggeinput: `build_linux_arm64_tls_attestation_candidate.no`,
  `native_metal_gpu_gate.no`, `windows_runtime_cross_compile_gate.no` kompilerer
  `build/v3009/native_candidate_gc.c` med `-Iarchive/legacy_c_backend`.
- `build/v3009/*.c` er allowlista i `verify_norscode_surface_ownership.no` og
  embedda av `tools/maint/refresh_embedded_runtime.no`.
- `startproject_templates/**payloads` er asserta av `release_preflight.no`.

Desse forsvinn difor **saman med milestone B** (når native codegen fullt erstattar
C-runtime + ABI-fasiten), ikkje i Fase 0.

**Faktisk fritt no:**
- ✅ `archive/legacy_c_backend/tools/ncb_to_c_host_v9300.py` — ureferert. Sletta.
  (Python i repoet: 1 → 0.)

**Krev grønt test-run før untrack (ikkje gjort blindt):**
- `examples*/**.ncb.json` + `tests*/**.ncb.json` (34 filer, 28 med `.no`-sysken):
  regenererbare artefaktar, men må stadfestast med full suite grøn før dei
  git-ignorerast — untrack utan verifisering kan gje raud CI.

- **Gate**: `ci.yml` + `selvstendighet.yml` framleis grøn (python-sletting rører dei ikkje).

### Fase 1 — Native seed som *komplett kompilator* (milestone B, den harde)
Blokkeringa som resten heng på. Seeden må kunne `run`/`kompiler` heil `.no`-kjelde
utan gcc/OpenSSL og utan å lene seg på ekstern precompiled-JSON.
- Fullfør x86_64 + ARM64 native codegen (null-representasjon-fiksen — sjå
  `docs/NORSDB_...`/native-codegen-notatane; krev gdb-diagnose av
  `emit_null_safe_index_set`-bruddet).
- Bygg seed med **innebygd** TEXT_V1-NCB (kompilator + stdlib), ikkje
  `NC_NATIVE_EXTERNAL_EMBED_NCB=1`.
- **Gate**: `bootstrap/stage0/norscode-linux-x86_64 run <kjelde>` +
  `test-parallel` + `bygg-native` grøn på alle fire plattformer.

### Fase 2 — Flytt runtime-lasting frå JSON → embedded/cache
- I `selfhost/vm.no`, `bundler.no`, `elf_compile_driver.no`: legg til ein
  oppslags-rekkjefølgje `embedded → build-cache → (feil)` i staden for
  `bootstrap/precompiled/*`.
- Introduser `build/cache/` (git-ignorert) som materialiseringsmål; eit
  `tools/materialize_bootstrap.no` regenererer det frå kjelde med seeden.
- **Gate**: kald checkout utan `bootstrap/precompiled/*` → `tools/nc_test.no` grøn
  (dvs. ingenting les den committede JSON-en lenger).

### Fase 3 — Snu self-sufficiency-gaten til reproduserbarheit
- Endre `tools/verify_selvstendighet.no`: i staden for «regenerer og samanlikn mot
  committa JSON», gjer «regenerer to gonger frå kjelde med seeden → krev
  byte-identisk», pluss «seeden reproduserer seg sjølv» (seed byggjer seed).
- Oppdater `selvstendighet.yml` deretter.
- **Gate**: `selvstendighet.yml` grøn utan å referere nokon `.ncb.json`.

### Fase 4 — Slett bootstrap-JSON
- `git rm bootstrap/precompiled/ bootstrap/precompiled_fragments*/ bootstrap/stdlib/*.ncb.json bootstrap/kompiler.ncb.json`.
- Rydd `tools/build_*_candidate.no` / `*regen*.no` som produserte JSON: peik dei mot
  build-cache i staden, eller fjern om overflødige.
- **Gate**: full CI-matrise grøn på ein checkout som inneheld **null** `*.ncb.json`
  (verifiser med `git ls-files "*.ncb.json" | wc -l == 0`).

### Fase 5 — Lås det inne
- Ny CI-gate `no-committed-bytecode`: feilar om `git ls-files "*.ncb.json"` er ikkje-tom.
- Utvid `tools/no_c_python_active_surface.no`-tankegangen: den aktive flata er rein
  `.no` + native binær-seed, ingenting anna.

## 5. Rekkefølgje og avhengnad

```
Fase 0 ─┬─▶ (uavhengig, kan gjerast no)
        │
Fase 1 ─┴─▶ Fase 2 ─▶ Fase 3 ─▶ Fase 4 ─▶ Fase 5
   ▲
   └── kritisk sti: heile JSON-avviklinga heng på ein komplett native seed
```

Fase 0 kuttar ~240 filer utan risiko og kan landast med det same. Fase 1–5 er
seriell og deler blokkering med milestone B (`test-parallel`/`bygg-native`).

## 6. Risiko

- **Tap av kjend-god fasit**: når JSON-fasiten forsvinn, blir reproduserbarheit den
  einaste garantien. Behald ein *tagga* git-revisjon med siste committede JSON som
  historisk anker (ikkje i HEAD, men gjenfinnbar).
- **Skjult JSON-avhengnad**: ~20 `tools/build_*`/`selfhost/*` les precompiled. Fase 2
  må sveipe alle (`grep -rl "precompiled\|\.ncb\.json"`) før Fase 4-sletting.
- **Seed-drift**: embedded bytekode i seeden må regenererast når kompilatoren endrar
  seg — Fase 3-reproduserbarheitsgaten fangar drift.

## 7. Akseptkriterium (heile programmet)

1. `git ls-files "*.ncb.json" | wc -l` → **0**.
2. Frisk checkout + berre `bootstrap/stage0/norscode-<plattform>` → full CI grøn.
3. `selvstendighet.yml` verifiserer seed-reproduserbarheit utan JSON-referanse.
4. `no-committed-bytecode`-gaten hindrar tilbakefall.

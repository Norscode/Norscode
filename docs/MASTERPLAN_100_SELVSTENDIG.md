# Masterplan: 100 % sjølvstendig Norscode (C + Python + JSON vekk, AOT-native)

**Mål:** Norscode byggjer, kompilerer, testar og køyrer seg sjølv **AOT-native**,
i rein `.no`-kjelde + native binær-seed, på fire plattformer, med **null C /
Python / shell / Zig / committa JSON** i den aktive flata — verifisert ved
reproduserbarheit (regenerer to gonger → byte-identisk) og signert attestasjon.

Bind saman tre eksisterande spor:
[AOT_OVERGANG_PLAN.md](AOT_OVERGANG_PLAN.md) ·
[JSON_AVVIKLING_PLAN.md](JSON_AVVIKLING_PLAN.md) ·
[SELVSTENDIGHET_SLUTTPLAN.md](SELVSTENDIGHET_SLUTTPLAN.md).

## Den store innsikta

**C-fjerning, JSON-fjerning og AOT er alle blokkerte på DET SAME:** ein
**komplett native seed** som kan kompilere heil `.no`-kjelde til native maskinkode
med full M1–M13-paritet, utan gcc/Zig/OpenSSL og utan ekstern precompiled-JSON.
Det er **Omgang 1–8** (milestone B) nedanfor. Klarar vi den, fell C, JSON og AOT
ut som følgje. Rekkefølgja: **Omgang 0 → 1–8 (flaskehals) → 9–18 → 19**.

---

## Omgang 0 — Baseline *(allereie i mål — ikkje rør)*

- [x] Python fjerna — 0 aktive `.py`.
- [x] Linux ELF (x86_64/arm64) + macOS Mach-O native-bygg C-frie («utan clang»).
- [x] Krypto/TLS i rein Norscode (D2: TLS 1.3 + web-PKI).
- [x] M1–M13 språkparitet i VM-tolk (kjelde) — **orakelet** omgangane under testar mot.

---

# ▓▓▓ MILESTONE B — komplett native seed (Omgang 1–8) · FLASKEHALSEN ▓▓▓

> Speglar AOT-plan omgang 0–5 + JSON-plan Fase 1. Sluttilstand: seeden er ein
> komplett AOT-kompilator med full paritet, byte-lik med tolken.

## Omgang 1 — Null-representasjon-fiks i codegen  ·  *1–2 v*

- [x] Diagnose + fiks av `INDEX_SET`-null-deref (signal 139): INDEX_GET var
      null-trygg, INDEX_SET ikkje. Ny `emit_null_safe_index_set` (x86_64) +
      `cbz Xobj,done`-vakt (ARM64). *(32c0693)*
- [x] Konsistent null-tryggleik i heile codegen: alle inlina container-deref-
      punkt vakta. x86_64 = `emit_null_safe_index_get/set` (map/list via RT-kall
      = runtime-lag). ARM64 = `emit_index_get/set` + `emit_map_get` + `emit_legg_til`
      (cbz-vakter). *(0b9ae37, 8df31bf; nc check OK)*
- **Port:** native regresjonssuite grøn på x86_64 + ARM64 — **verifiserast av
      PR #184 sin CI** (ELF stage-0 alt grøn 2×; Native-jobbane er den siste biten).
      *Ikkje stadfestbart frå macOS; maskinkoden er byte-usjekka herifrå.*

## Omgang 2 — Differensial-sele + opcode-matrise  ·  *1 v*

- [x] Differensial-sele som køyrer AOT + tolk og feilar ved ulikt resultat:
      `tools/differensial_sele.no` (returverdi/exit-kode-samanlikning) +
      fixturar (`diff_smoke`, `diff_null_index`) + **CI-jobb «Differensial-sele
      (AOT vs tolk)»** på Linux. Tolk-delen verifisert lokalt (=42).
      *(0a90c7c, ca05609)*
- [x] Full opcode-støttematrise: `tools/opcode_matrise.no` (regenererbar) +
      `docs/OPCODE_MATRISE.md`. Viser tolk × AOT-x86 × AOT-arm64. Funn: M3
      (BUILD_LAMBDA/CALL_VALUE) manglar overalt, unntak/felt berre tolk,
      TRY_*/LOAD_EXCEPTION manglar i AOT-x86. *(f844c08)*
- **Port:** differensial-selen grøn for alle opcodane AOT alt støttar
      — CI-jobben er verifiseringa (tolk-delen lokalt grøn).

## Omgang 3 — M3 i AOT: closures/lambdaer  ·  *3–5 v · tyngst*

- [x] Closure-representasjon i heap (funksjonsindeks + capture-peikar, ABI-dok):
      tagg 6 = `{fn_index, capture_ptr}`, funksjons-adressetabell (indeks→adresse
      via `adrp+add`), capture som etterfølgjande arg `x[argc]`, prolog-binding
      via `map_get`. Grunna i faktisk NcVal-/funksjonslayout, differensial-kontrakt
      mot tolk-orakelet. **[docs/OMGANG3_CLOSURE_ABI.md](OMGANG3_CLOSURE_ABI.md)**
- [ ] `BUILD_LAMBDA` + `CALL_VALUE` i native codegen (indirekte kall).
- [ ] Capture-binding i callee-prolog + fixpunkt-regenerering av seed.
- **Port:** `test_lambda_closure` AOT == tolk (differensial grøn).

## Omgang 4 — M5 i AOT: struktur-metodar  ·  *2 v*

- [ ] `__struct_type__`-tagging i native konstruktørar.
- [ ] `kall_metode`-dispatch i native (eksakt/suffiks `<type>.<metode>`).
- **Port:** `test_metode` + `test_grensesnitt`-dispatch AOT == tolk.

## Omgang 5 — M10 i AOT: varargs, standardarg, spread, destrukturering  ·  *2–3 v*

- [ ] Rest-param → liste i native prolog; standardargument-prolog.
- [ ] Spread/destrukturering/optional/template i native.
- **Port:** `test_varargs/standardarg/spread/destrukturering/optional/template` AOT == tolk.

## Omgang 6 — M2/M4/M6/M9 i AOT  ·  *2–3 v*

- [ ] Fangbare feil + stack traces (native `vm_med_spor`-motstykke); `ingenting`-type;
      unntakstypar/hierarki; enum-konstantar + grensesnitt-kontraktsjekk.
- **Port:** M2/M4/M6/M9-testane AOT == tolk.

## Omgang 7 — Full runtime-paritet  ·  *1–2 v*

- [ ] Køyr heile `std/`- + `selfhost/`-testflata gjennom differensial-selen; tett kvart avvik.
- [ ] M1 strict typesjekk verkar likt; alle builtins dekt i AOT.
- **Port:** **null differensial-avvik** over heile testflata.

## Omgang 8 — Innebygd seed  ·  *2–3 v*

- [ ] Bygg seed med TEXT_V1-NCB (kompilator + stdlib) **embedda** i binæren,
      ikkje `NC_NATIVE_EXTERNAL_EMBED_NCB=1`.
- **Port (MILESTONE B):** `stage0 run <kjelde>` + `test-parallel` + `bygg-native`
      grøn på alle fire plattformer; ELF stage-0-fixpunkt (Gen1==Gen2) grøn.
      **← Flaskehalsen passert; Omgang 9–18 kan gå parallelt.**

---

# ▓▓▓ Opprydding etter milestone B (Omgang 9–18) ▓▓▓

## Omgang 9 — Windows PE-codegen i rein Norscode  ·  *3–4 v*  *(C + AOT-plattform)*

- [ ] PE-emitter i rein Norscode → **erstatt `zig cc`**
      (`windows_runtime_cross_compile_gate.no`).
- [ ] Windows-thread-backend + freestanding ARM64/Windows-atomics.
- **Port:** Windows native ELF/PE bygd og køyrd utan Zig; differensial grøn.

## Omgang 10 — Erstatt OpenSSL / SQLite / Zig + native ABI  ·  *3–5 v*  *(C)*

- [ ] Fjern OpenSSL/SQLite/Zig frå normalkandidatane (native Norscode-erstatning).
- [ ] Attståande filesystem-/nettverks-/sikkerheits-ABI etter promoteringsregelen.
- **Port:** normalkandidatar på alle plattformer utan OpenSSL/SQLite/Zig.

## Omgang 11 — Fjern legacy C-backend ABI-fasit  ·  *1–2 v*  *(C)*

- [ ] Fjern `archive/legacy_c_backend`-avhengnaden — native codegen erstattar
      C-runtime + dei ~20 `release_preflight`-assertane som les `.c`.
- **Port:** `no_c_python_active_surface` grøn med **0** aktive `.c/.h/.py/.sh`/Zig.

## Omgang 12 — JSON: runtime-lasting embedded → build-cache  ·  *2 v*  *(JSON)*

- [ ] `vm.no`/`bundler.no`/`elf_compile_driver.no`: oppslag `embedded → build/cache/ → feil`,
      aldri `bootstrap/precompiled/*`. Legg til `tools/materialize_bootstrap.no`.
- **Port:** kald checkout utan `bootstrap/precompiled/*` → `nc_test.no` grøn.

## Omgang 13 — JSON: gate → reproduserbarheit  ·  *1–2 v*  *(JSON)*

- [ ] `verify_selvstendighet.no`: «regenerer to gonger frå kjelde → byte-identisk»
      + «seed byggjer seed», i staden for samanlikning mot committa JSON.
- **Port:** `selvstendighet.yml` grøn utan å referere nokon `.ncb.json`.

## Omgang 14 — JSON: slett bootstrap-JSON + lås  ·  *1 v*  *(JSON)*

- [ ] `git rm bootstrap/precompiled*` + alle committa `*.ncb.json`; regen-verktøy → build-cache.
- [ ] CI-gate `no-committed-bytecode` (feilar om `git ls-files "*.ncb.json"` ikkje-tom).
- **Port:** full CI-matrise grøn med `git ls-files "*.ncb.json" | wc -l == 0`.

## Omgang 15 — Køyremodell-byte: `nc run` → AOT  ·  *2–3 v*  *(AOT)*

- [ ] `nc run` AOT-kompilerer + køyrer native; tolk berre fallback med åtvaring;
      AOT-artefakt-cache.
- **Port:** `nc run` på heile testflata via AOT; ingen fallback-varsel att.

## Omgang 16 — Sjølvhost fixpunkt på AOT  ·  *2–3 v*  *(AOT)*

- [ ] Seed AOT-kompilert m/full paritet; Gen1==Gen2==Gen3; tolken sjølv AOT-kompilert.
- **Port:** ELF stage-0 + `selvstendighet --strict` grøn med full-paritet-AOT-seed.

## Omgang 17 — Ytelse  ·  *3–6 v*  *(AOT)*

- [ ] Register-allokering, inlining, dead-code/konstant-folding.
- **Port:** `nc benchmark` viser AOT ≥5× raskare enn tolk på kjernesuiten.

## Omgang 18 — Tolk valfri, AOT standard  ·  *1–2 v*  *(AOT)*

- [ ] VM-tolk berre for `eval`/REPL/debug/differensial; AOT standard for run/test/serve/CI.
- **Port:** full sjølvhost + testflate + CI grøn **utan** tolk-fallback.

---

## Omgang 19 — Lås inne: 100 % sjølvstendig

- [ ] Aktiv-flate-gate: **0** `.c/.h/.py/.sh`/Zig i aktiv verktøykjede.
- [ ] Bytekode-gate: **0** committa `*.ncb.json`.
- [ ] Køyremodell: AOT ende-til-ende.
- [ ] Reproduserbarheit: regenerer 2× → byte-identisk; seed byggjer seed.
- [ ] Signert attestasjon: macOS + Linux x86-64/ARM64 + Windows.
- [ ] ELF stage-0-fixpunkt grøn med full-paritet-AOT-seed.

---

## Definisjon av 100 % ferdig

Norscode byggjer, kompilerer, testar og køyrer seg sjølv **AOT-native** frå rein
`.no`-kjelde + éin native binær-seed, på macOS + Linux x86-64/ARM64 + Windows, med
**null C / Python / shell / Zig / committa JSON** i den aktive flata, verifisert
ved reproduserbarheit + signert attestasjon.

## Rekkefølgje og parallellisme

```
Omgang 0 ✅
   └─▶ 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8   (MILESTONE B — sekvensiell, flaskehals)
          └─▶ etter 8, parallelt:
                C-spor:   9 → 10 → 11
                JSON-spor:12 → 13 → 14
                AOT-spor: 15 → 16 → 17 → 18
   Alle tre spor ferdige ──▶ 19 (lås inne)
```

**Alt startar med Omgang 1–8.** Utan milestone B står C-, JSON- og AOT-spora stille.

## Notat om noverande arbeid

- **VM-tolk-paritet (PR #182)** er Omgang 0-orakelet. M3 CALL_VALUE-tapet i den
  rebasa `vm.no` bør reparerast så tolken er komplett før Omgang 3 differensial-testar mot han.
- **Fixpunkt-iterasjonane** du gjer manuelt er ein forsmak på Omgang 13 (reproduserbarheit).

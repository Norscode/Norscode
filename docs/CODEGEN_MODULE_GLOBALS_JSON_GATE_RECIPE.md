# Oppskrift: lever module-globals JSON-gate inn i aktiv codegen-NCB

Formål: `native_codegen_v2.bygg_runtime_v2` kaster «venta 1 legacy JSON-literal,
fann 0» for minimale program (modul-globals utan JSON-op). Fiksen gater dei seks
`patch_legacy_json_literal`-kalla på om JSON-runtime-sekvensen faktisk finst.

**Viktig:** codegen kjøyrer frå ein **bundlet/embedded NCB**, ikkje frå kjelde.
`nc run` + `NORSCODE_USE_PRECOMPILED_SELFHOST=0` + cache-tømming tek IKKJE
kjeldeendringa. Difor må fiksen leverast via `extract_source_function_candidate`
+ `overlay_ncb_function_candidate`, på ein **hex-kapabel host** (ikkje seed-stage0
— den gjer `0x`-literalar til `ingenting`).

## Kjeldeendringa (alt lagt inn i `selfhost/native_execution/native_codegen_v2.no`)

1. Ny hjelpar `har_legacy_json_literal(bytes, peikar_låg)` (rett etter
   `patch_legacy_json_literal`) — ikkje-muterande mønstersøk.
2. I `bygg_runtime_v2`: dei seks `patch_legacy_json_literal`-kalla er no gata i
   `hvis json_runtime_present { ... }`.

Dette er den kanoniske kjelda å ekstrahere frå. `nc run` reflekterer det ikkje;
det er overlay-steget som gjer det aktivt.

## Steg 1 — finn den aktive base-codegen-NCB-en

```bash
# NCB-en som runtime faktisk lastar native_codegen_v2 frå (inneheld funksjonane):
grep -l 'native_execution.native_codegen_v2.bygg_runtime_v2' build/*.ncb.json
# Vel den aktive (den dist/bin/nc faktisk brukar). Kall han BASE nedanfor.
BASE=build/<aktiv_codegen>.ncb.json
```

## Steg 2 — ekstraher dei to endra funksjonane frå kjelde

```bash
NC_SOURCE_INPUT=selfhost/native_execution/native_codegen_v2.no \
NC_SOURCE_OUTPUT=build/cand_har_legacy.no \
NC_SOURCE_FUNCTION=har_legacy_json_literal \
NC_SOURCE_INCLUDE_IMPORTS=1 \
./bin/nc run tools/extract_source_function_candidate.no

NC_SOURCE_INPUT=selfhost/native_execution/native_codegen_v2.no \
NC_SOURCE_OUTPUT=build/cand_bygg_runtime_v2.no \
NC_SOURCE_FUNCTION=bygg_runtime_v2 \
NC_SOURCE_INCLUDE_IMPORTS=1 \
./bin/nc run tools/extract_source_function_candidate.no
```

## Steg 3 — kompiler kandidatane til NCB

```bash
NORSCODE_CMD=compile NORSCODE_FILE=build/cand_har_legacy.no \
NORSCODE_MODULE=selfhost.native_execution.native_codegen_v2 \
NORSCODE_OUTPUT=build/cand_har_legacy.ncb.json ./bin/nc

NORSCODE_CMD=compile NORSCODE_FILE=build/cand_bygg_runtime_v2.no \
NORSCODE_MODULE=selfhost.native_execution.native_codegen_v2 \
NORSCODE_OUTPUT=build/cand_bygg_runtime_v2.ncb.json ./bin/nc
```

(Bruk same kompilerings-kommando som resten av NCB-arbeidsflyten din om denne
skil seg.)

## Steg 4 — overlay inn i BASE (først ADD hjelpar, så REPLACE byggjar)

```bash
# 4a: legg til den nye funksjonen har_legacy_json_literal
NC_OVERLAY_BASE="$BASE" \
NC_OVERLAY_SOURCE=build/cand_har_legacy.ncb.json \
NC_OVERLAY_OUT=build/codegen_overlay_step1.ncb.json \
NC_OVERLAY_BASE_FUNCTION=selfhost.native_execution.native_codegen_v2.har_legacy_json_literal \
NC_OVERLAY_SOURCE_FUNCTION=selfhost.native_execution.native_codegen_v2.har_legacy_json_literal \
NC_OVERLAY_ADD_FUNCTION=1 \
./bin/nc run tools/overlay_ncb_function_candidate.no

# 4b: erstatt bygg_runtime_v2 med den gata versjonen
NC_OVERLAY_BASE=build/codegen_overlay_step1.ncb.json \
NC_OVERLAY_SOURCE=build/cand_bygg_runtime_v2.ncb.json \
NC_OVERLAY_OUT=build/codegen_overlay_step2.ncb.json \
NC_OVERLAY_BASE_FUNCTION=selfhost.native_execution.native_codegen_v2.bygg_runtime_v2 \
NC_OVERLAY_SOURCE_FUNCTION=selfhost.native_execution.native_codegen_v2.bygg_runtime_v2 \
NC_OVERLAY_NORMALIZE_CALLS=1 \
./bin/nc run tools/overlay_ncb_function_candidate.no
```

`build/codegen_overlay_step2.ncb.json` er no den patcha codegen-kandidaten,
utan at BASE, runtime eller stage0 er rørt.

## Steg 5 — verifiser på hex-kapabel host

Peik runtime på den overlaya codegen-kandidaten (same mekanisme du elles brukar
for å teste ein codegen-NCB-kandidat), og kjør:

```bash
./bin/nc run tests/test_native_module_globals_codegen.no      # skal no vere grøn
```

Regresjon: kjør ein test som FAKTISK bruker JSON gjennom same codegen-kandidat —
gaten skal berre hoppe over når INGEN JSON-sekvens finst; JSON-program må framleis
patche alle seks (delvis mønster feilar lukka).

## Om fiksen viser seg feil

Om «fann 0» kjem av at di lokale endring i sjølve hex-malen i `bygg_runtime_v2`
fjerna JSON-hjelparen for alle program (ikkje berre minimale), er gaten feil
kompensasjon — då skal hex-malen rettast i staden. Verifiser med eit JSON-program
(regresjonen over): er den grøn, er gaten rett; feilar den, ligg årsaka i malen.

## Promoteringsregel

Dette er tillitsanker-nær codegen. Følg kandidat → levande smoke → full matrise →
kontrollert atomisk promotering. Ikkje promoter `build/codegen_overlay_step2.ncb.json`
til aktiv codegen før både module-globals-testen og eit representativt JSON-program
er grøne på hex-kapabel host/CI.

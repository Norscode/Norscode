# v3007 Active runtime status

## Current status on 2026-06-27

Active dist and host-stage0 are now verified green by:

```sh
/Users/jansteinar/Norscode\ AI/prosjekter/NorscodeAIKernel/tools/production_readiness_gate.sh --full
```

The older v3007 status below is historical context from before active dist/stage0 gates were added to the AIKernel production matrix.

Status: aktiv runtime-status er skilt frå kandidat-status.

## Kvifor

Etter v3002-v3006 har vi ein grønn macOS-kandidat, men aktiv runtime er ikkje automatisk endra. Det er viktig at dokumentasjon og testkommandoar ikkje blandar desse to statusane.

## Definisjonar

- Kandidat: `build/v3002/norscode_native_v3002`
- Aktiv dist: `dist/norscode_native`
- Stage0: `bootstrap/stage0/norscode-<plattform>`

## Script

```sh
./bin/nc run tools/native_active_status_v3007.no
```

Scriptet:

- viser kandidatfil
- viser aktiv `dist/norscode_native`
- viser sha256 når filene finst
- skriv kommando for kandidat-gate
- skriv kommando for aktiv dist-gate
- endrar ingen filer
- promoterer ingenting

## Historical snapshot etter v3007 (ikkje aktuell release-status)

Dette er eit historisk kontrollpunkt frå før den versjonerte sluttporten vart
innført. Det skal ikkje brukast som aktuell readiness-attestasjon. Les
`reports/norscode-completion-v1.json` frå same commit som kjelda for gjeldande
status.

```text
candidate_runtime_gap: green
active_dist_runtime_gap: not_claimed
stage0_runtime_gap: not_claimed
production_ready: false
reason: active_runtime_not_promoted
```

## Viktig regel

Det er lov å seie:

```text
random_hex fungerer i v3002-kandidaten.
```

Det er ikkje lov å seie:

```text
random_hex fungerer i aktiv dist.
```

før `NORSCODE_NATIVE_GAP_BIN=dist/norscode_native ./bin/nc run tools/native_runtime_gap_gate_v3001.no` er køyrt og grønn etter eksplisitt promotering.

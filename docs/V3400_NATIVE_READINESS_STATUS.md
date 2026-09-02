# v3400 Native readiness status

Status: kandidat er promotert til aktiv dist og macOS stage0.

## Samandrag

I v3201-v3400 var målet å gjere promoteringa trygg og forståeleg. Etter eksplisitt godkjenning er macOS-kandidaten promotert til aktiv `dist/norscode_native` og `bootstrap/stage0/norscode-macos-arm64`.

- dist dry-run er grønn
- stage0 dry-run er grønn
- aktiv `dist/norscode_native` er kontrollert separat
- aktiv `dist/norscode_native` er grønn etter runtime-gap gate
- `bootstrap/stage0/norscode-macos-arm64` er grønn etter runtime-gap gate

## Faktisk status

### Kandidat

- Fil: `build/v3002/norscode_native_v3002`
- SHA256: `242c26c3aa4e3caa1f76aa81113bf0dbc7fd7070b6d799bbcd0759380409fa36`
- Runtime-gap gate: grønn
- Dist dry-run: grønn
- Stage0 dry-run: grønn

### Aktiv dist

- Fil: `dist/norscode_native`
- SHA256: `242c26c3aa4e3caa1f76aa81113bf0dbc7fd7070b6d799bbcd0759380409fa36`
- Runtime-gap gate: grønn
- Promotert: 2026-06-25T15:17:55Z

Verifisert i aktiv dist:

```text
random_hex OK
tid_ms OK
tid_no OK
now OK
timestamp OK
exec_prosess OK
```

### Stage0

- Fil: `bootstrap/stage0/norscode-macos-arm64`
- SHA256: `242c26c3aa4e3caa1f76aa81113bf0dbc7fd7070b6d799bbcd0759380409fa36`
- Runtime-gap gate: grønn
- Promotert: 2026-06-25T15:19:03Z

## Produksjonsstatus

```text
candidate_runtime_gap=green
active_dist_runtime_gap=green
stage0_runtime_gap=green
stage0_promoted=true
production_ready_macos=true
production_ready_all_platforms=false
reason=macos_dist_and_stage0_runtime_gap_green_linux_not_built_here
```

## Kva dette betyr

Det er no korrekt å seie:

```text
Aktiv dist og macOS stage0 er grøne for runtime-gap.
```

Det er framleis ikkje målt her:

```text
Linux stage0 er ferdig.
```

## Neste steg

- v3401-v3600: Linux-kandidat på Linux
- v3601-v3700: random og tid API-dokumentasjon etter aktiv macOS-runtime

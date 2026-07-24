# V4200 Runtime Basics Status

Dato: 2026-06-25
Fase: v3901-v4200 fil/json/map/tekst stabilisering

## Sluttstatus

Runtime-basics er grøne på:
- aktiv macOS `dist/norscode_native`
- aktiv Linux x86_64 `bootstrap/stage0/norscode-linux-x86_64`

## Verifisert grønt

Tekst:
- `trim`
- `split`
- `starts_with`
- `ends_with`
- `tekst_fra_heltall`

JSON og map:
- `json_parse_raw`
- `json_stringify`
- `nøkler`
- `verdier`

Fil:
- `fil_skriv`
- `fil_les`

## Probe-oppsett

Probe:
- `/Users/jansteinar/Norscode AI/prosjekter/Norscode/tests/runtime_basics_probe_v4200.no`

Fixture:
- `/Users/jansteinar/Norscode AI/prosjekter/Norscode/tests/fixtures/runtime_basics_v4200.json`

Køyrescript:
- `/Users/jansteinar/Norscode AI/prosjekter/Norscode/tools/run_runtime_basics_phase_v4200.no`

## Viktig læring

Den første feiltolkinga kom frå probe-strengen for JSON, ikkje frå sjølve runtime.
Ved å lese ekte JSON frå fixture-fil fekk vi verifisert at `json_parse_raw`, `nøkler`, `verdier` og `json_stringify` faktisk verkar i aktiv runtime og Linux-stage0.

## Konklusjon

`runtime_basics_phase=green`

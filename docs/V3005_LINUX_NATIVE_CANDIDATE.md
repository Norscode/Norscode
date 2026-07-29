# v3005 Linux native candidate

Status: Linux-kandidatspor er klargjort, men ikkje køyrt på macOS.

## Formål

v3005 skil Linux-kandidaten frå macOS-kandidaten. Scriptet skal berre køyre på Linux og skal aldri promotere direkte til `dist` eller `bootstrap/stage0`.

## Script

```sh
./bin/nc run tools/build_linux_native_candidate_v3005.no
```

## Reglar

- Stoppar trygt på macOS.
- Byggjer berre kandidat under `build/v3002/`.
- Køyrer `tools/native_runtime_gap_gate_v3001.no` mot kandidaten.
- Skriv manifest til `build/v3002/native_linux_candidate_v3005.manifest` berre når Linux-build og gate er grønn.
- Endrar ikkje `dist/norscode_native`.
- Endrar ikkje `bootstrap/stage0`.

## Kva må vere grønt på Linux

```text
random_hex OK
tid_ms OK
tid_no OK
now OK
timestamp OK
exec_prosess OK
```

`exec_prosess` er framleis DEV-gated gjennom `NORSCODE_ENABLE_EXEC_PROSESS=1` i runtime-gap-gaten.

## Neste steg

Etter grønn Linux-kandidat kan v3006 lage separat promotering-status for Linux, framleis utan automatisk overskriving av stage0.

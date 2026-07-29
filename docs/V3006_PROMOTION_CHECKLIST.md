# v3006 Native promotion checklist

Status: promotering er definert, men ikkje utført.

## Mål

Dette dokumentet er sperra mellom grønn kandidat og aktiv runtime. Det skal vere umogleg å forveksle:

- macOS-kandidat
- Linux-kandidat
- `dist/norscode_native`
- `bootstrap/stage0`

## Kjende kandidatar

### macOS arm64

- Status: grønn kandidat
- Fil: `build/v3002/norscode_native_v3002`
- Manifest: `build/v3002/native_candidate_v3003.manifest`
- SHA256: `242c26c3aa4e3caa1f76aa81113bf0dbc7fd7070b6d799bbcd0759380409fa36`
- Gate: grønn med `tools/native_runtime_gap_gate_v3001.no`
- Promotert: nei

### Linux

- Status: byggespor klart, ikkje køyrt her
- Script: `tools/build_linux_native_candidate_v3005.no`
- Manifest etter grønn Linux-build: `build/v3002/native_linux_candidate_v3005.manifest`
- Promotert: nei

## Før dist-promotering

Alle punkt må vere sanne:

- Kandidaten er bygd på same plattform som han skal brukast på.
- `NORSCODE_NATIVE_GAP_BIN=<kandidat> ./bin/nc run tools/native_runtime_gap_gate_v3001.no` er grønn.
- `random_hex OK` står i gate-output.
- `tid_ms OK` står i gate-output.
- `tid_no OK` står i gate-output.
- `now OK` står i gate-output.
- `timestamp OK` står i gate-output.
- `exec_prosess OK` står i gate-output med DEV-gate.
- Brukaren har sagt eksplisitt ja til dist-promotering.

Tørrkøyring:

```sh
NORSCODE_PROMOTE_CANDIDATE=build/v3002/norscode_native_v3002 NORSCODE_PROMOTE_DIST=1 NORSCODE_PROMOTE_DRY_RUN=1 ./bin/nc run tools/promote_native_stage0_v3001.no
```

Reell dist-promotering krev eksplisitt ja:

```sh
NORSCODE_PROMOTE_CANDIDATE=build/v3002/norscode_native_v3002 NORSCODE_PROMOTE_DIST=1 ./bin/nc run tools/promote_native_stage0_v3001.no
```

## Før stage0-promotering

Stage0 er strengare enn `dist`. Alle punkt må vere sanne:

- Dist-kandidaten har vore grønn.
- Kandidaten er bygd på målplattform.
- Plattformnamnet er kontrollert:
  - macOS arm64: `bootstrap/stage0/norscode-macos-arm64`
  - macOS x86_64: `bootstrap/stage0/norscode-macos-x86_64`
  - Linux x86_64: `bootstrap/stage0/norscode-linux-x86_64`
  - Linux arm64: `bootstrap/stage0/norscode-linux-arm64`
- Brukaren har sagt eksplisitt ja til stage0-promotering.
- Det finst backup-plan dersom aktiv bootstrap feilar.

Tørrkøyring:

```sh
NORSCODE_PROMOTE_CANDIDATE=build/v3002/norscode_native_v3002 NORSCODE_PROMOTE_STAGE0=1 NORSCODE_PROMOTE_DRY_RUN=1 ./bin/nc run tools/promote_native_stage0_v3001.no
```

Reell stage0-promotering krev eksplisitt ja:

```sh
NORSCODE_PROMOTE_CANDIDATE=build/v3002/norscode_native_v3002 NORSCODE_PROMOTE_STAGE0=1 ./bin/nc run tools/promote_native_stage0_v3001.no
```

## Historisk produksjonsstatus

Blokka under er eit eldre kontrollpunkt frå før promoteringa og sluttporten.
Ho skal ikkje lesast som aktuell status. Gjeldande status kjem berre frå den
signerte `norscode-completion-gate-v1`-rapporten på same commit.

Før reell promotering:

```text
production_ready: false
reason: candidate_not_promoted
```

Etter reell promotering må status oppdaterast separat og verifiserast mot aktiv `dist/norscode_native`.

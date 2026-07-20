# v3004 Native promotion dry-run

Status: promotering er klargjort, men ikkje utført.

## Formål

v3004 legg til trygg tørrkøyring for native-kandidaten. Dette gjer at ein kan sjekke runtime-gap-gate og sjå kva filer som ville blitt oppdatert, utan å skrive til aktiv runtime.

## Kommando

```sh
NORSCODE_PROMOTE_CANDIDATE=build/v3002/norscode_native_v3002 NORSCODE_PROMOTE_DIST=1 NORSCODE_PROMOTE_DRY_RUN=1 ./bin/nc run tools/promote_native_stage0_v3001.no
```

Valfritt for stage0-status:

```sh
NORSCODE_PROMOTE_CANDIDATE=build/v3002/norscode_native_v3002 NORSCODE_PROMOTE_STAGE0=1 NORSCODE_PROMOTE_DRY_RUN=1 ./bin/nc run tools/promote_native_stage0_v3001.no
```

## Sikkerheitsreglar

- `--dry-run` køyrer runtime-gap-gate.
- `--dry-run` skriv ikkje til `dist/norscode_native`.
- `--dry-run` skriv ikkje til `bootstrap/stage0`.
- Reell promotering krev framleis eksplisitt flagg, til dømes `--dist`, og skal ikkje køyrast utan eksplisitt ja.

## Kandidat frå v3003

- Fil: `build/v3002/norscode_native_v3002`
- SHA256: `242c26c3aa4e3caa1f76aa81113bf0dbc7fd7070b6d799bbcd0759380409fa36`
- Gate-status: grønn

## Status

`dist/norscode_native` og `bootstrap/stage0` er framleis urørte etter v3004.

# Selfhost diagnostics

Mål:
Gjøre det enkelt å forstå hva som skjer i et prosjekt uten å lese kildekoden først.

## Diagnosekommandoen

`./bin/nc doctor` er den normale diagnose-/verifiseringskommandoen i dagens CLI.

Kjør:

```bash
./bin/nc doctor
```

Diagnosen er ment for:

- rask status på et checkout eller installert miljø
- feilsøking av config, paths og testdiscovery
- enkel driftssjekk av hva som faktisk er tilgjengelig

## Hva den viser

### Human output

- om diagnosen er OK
- hvilken sti som ble sjekket
- om prosjektrot og config ble funnet
- prosjektets navn og entrypoint hvis de finnes
- sentrale paths (`source`, `stdlib`, `build`)
- stdlib-roots
- om `std.web` er resolvable
- dependency-count og test-count
- git-branchnet, dirty-state og revision
- en kort liste over de første testfilene

## Hvordan bruke den

- Bruk `./bin/nc doctor` når du vil forstå kvifor eit miljø ser annleis ut enn forventa.
- Kombiner gjerne med `./bin/nc test` og `bash tools/verify_selvstendighet.sh` når du vil verifisere både prosjektstatus og bootstrap-/releaseflate.

## Relatert

- [`docs/OBSERVABILITY_PATTERN.md`](./OBSERVABILITY_PATTERN.md)
- [`docs/QUALITY.md`](./QUALITY.md)
- [`docs/SELFHOST_CI_GATES.md`](./SELFHOST_CI_GATES.md)

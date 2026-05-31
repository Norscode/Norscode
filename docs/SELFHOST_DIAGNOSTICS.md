# Selfhost diagnostics

Mål:
Gjøre det enkelt å forstå hva som skjer i et prosjekt uten å lese kildekoden først.

## Diagnosekommandoen

`norcode diagnose` er en normal modulær CLI-kommando i `norcode/cli.py`.

Kjør:

```bash
norcode diagnose
```

eller som JSON for verktøy:

```bash
norcode diagnose --json
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

### JSON output

`--json` gir et stabilt objekt som er egnet for verktøy og CI:

- `ok`
- `root`
- `project_root`
- `config_path`
- `project_name`
- `project_entry`
- `paths`
- `stdlib_roots`
- `stdlib_resolves_web`
- `dependency_count`
- `dependencies`
- `test_count`
- `tests`
- `git`

## Hvordan bruke den

- Bruk `norcode diagnose` når du vil forstå hvorfor et miljø ser annerledes ut enn forventet.
- Bruk `norcode diagnose --json` når et script eller en pipeline skal lese resultatet maskinelt.
- Kombiner gjerne med `doctor` når du vil verifisere installasjon og release i tillegg til prosjektstatus; `doctor` er fortsatt den eksplisitte verifiseringsflaten for release-/installasjonsmiljøet.

## Relatert

- [`docs/OBSERVABILITY_PATTERN.md`](./OBSERVABILITY_PATTERN.md)
- [`docs/QUALITY.md`](./QUALITY.md)
- [`docs/SELFHOST_CI_GATES.md`](./SELFHOST_CI_GATES.md)

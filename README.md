# Norscode

Norscode er eit norsk språk- og verktøysett med native-first CLI, selfhost-løype og ei aktiv flate utan Python eller C.

![Norscode-oversikt](docs/assets/norscode-overview.svg)

![Norscode-logo](frontend/assets/icons/norscode-logo-dark.svg)

## Kort fortalt

- Norsk syntaks for funksjonar, kontrollflyt og uttrykk
- Statisk typing for heiltal, tekst, bool, lister og ordbøker
- Modul- og pakke-system
- Standardbiblioteket `std`
- Feilhandtering med `kast`, `prøv` og `fang`
- Normal kjede: `.no` -> NCB JSON -> `selfhost/vm.no`

## Kom i gang

1. Les [dokumentasjonsinngangen](docs/INDEX.md)
2. Installer med [installasjonsguiden](INSTALL.md)
3. Bruk [brukarmanualen](docs/USER_MANUAL.md) som praktisk manual
4. Følg [opplæringsguiden](docs/LEARNING_GUIDE.md) for opplæring
5. Bruk [løypekartet](docs/LANE_MAP.md) for rett arbeidsløype
6. Les [selfhost-handlingsplanen](docs/SELFHOST_HANDLINGSPLAN.md) for selfhost-løypa
7. Sjekk [statussida](docs/STATUS.md)
8. Vedlikehald: `./bin/nc maintenance verify`

Snarvegar:

```bash
./bin/nc --help
./bin/nc run app.no
./bin/nc check app.no
./bin/nc feature-check app.no
./bin/nc repl '1 + 2'
./bin/nc release-preflight
./bin/nc release-preflight --strict
./bin/nc local-green
./bin/nc local-green --strict
./bin/nc local-green --strict --list
./bin/nc local-green --strict -l
./bin/nc local-green --list
./bin/nc local-green -l
./bin/nc local-green --help
./bin/nc local-green -h
./bin/nc stage0-release-assets --platform macos-arm64
./bin/nc test
```

## Normal bruk

- `./bin/nc` og `dist/norscode_native` er normal CLI og runtime
- `./bin/nc run <fil.no>` køyrer også vanlege `bruk`/`importer`-program med bundla imports
- `./bin/nc feature-check [fil.no ...]` er standard gate for å byggje nye funksjonar direkte i Norscode
- `./bin/nc repl [uttrykk]` evaluerer små uttrykk direkte eller startar interaktiv REPL
- `./bin/nc release-preflight` sjekkar release-/GitHub-klargjering lokalt utan å publisere
- `./bin/nc release-preflight --strict` er siste lokale port før GitHub/release og feilar på uspora nøkkelfiler
- `./bin/nc local-green` køyrer lokal release-preflight, aktiv-flate-sjekk, fase-0, L1-L6-sjølvstendighet og full testflate utan å publisere
- `./bin/nc local-green --strict` køyrer same grønnliste med streng release-preflight som fyrste steg
- `./bin/nc local-green --strict --list` eller `./bin/nc local-green --strict -l` viser streng grønnliste med steg og kommandoar utan å køyre henne
- `./bin/nc local-green --list` viser grønnliste-stega med kommandoar utan å køyre dei
- `./bin/nc local-green -l` er kortform for `--list`
- `./bin/nc local-green --help` viser kort brukshjelp utan å køyre grønnlista
- `./bin/nc local-green -h` er kortform for `--help`
- `./bin/nc stage0-release-assets --platform <plattform>` byggjer stage-0 release-ELF og flyttbar `.sha256` under `release-artifacts/stage0/`
- `./bin/nc maintenance verify` gir vedlikehaldssamandrag
- `./bin/nc maintenance status`, `lane`, `seed`, `seed-status` viser Norscode-vedlikehald og stage-0-status
- `./bin/nc maintenance report-json` gir maskinlesbar statusrapport (inkl. `stage0_seed_ok`)
- `./bin/nc selvstendighet` verifiserer normalflata utan C-regen eller stage-0 rebuild

## Dokumentasjon

- [Installasjon](INSTALL.md)
- [Dokumentasjonsinngang](docs/INDEX.md)
- [Brukarmanual](docs/USER_MANUAL.md)
- [Opplæringsguide](docs/LEARNING_GUIDE.md)
- [Dokumentasjonsindeks for vedlikehald](docs/DOCUMENTATION_INDEX.md)
- [Løypekart](docs/LANE_MAP.md)
- [Merkevare og ikon](docs/BRAND.md)
- [Selfhost-handlingsplan](docs/SELFHOST_HANDLINGSPLAN.md)
- [Status](docs/STATUS.md)

## Verifisering

- `./bin/nc test` går grønt med reelle tal for bestått/hoppa/feila testar
- `./bin/nc release-preflight` går grønt før tag/release og publiserer ingenting
- `./bin/nc release-preflight --strict` skal vere grøn før push/tag når nye nøkkelfiler skal med til GitHub
- `./bin/nc local-green` er samla lokal grønnliste før tag, release eller større rydding
- `./bin/nc local-green --strict` er samla streng grønnliste før push/tag
- L1-L6/selfhost-verifisering går grønt via `./bin/nc selvstendighet`
- aktiv verktøyflate er fri for Python og C; historisk C ligg berre under `archive/`
- aktiv ikkje-Norscode plattformkode er avgrensa til dokumenterte OS-bruer under `platform/`

## Sjølvstendighet akkurat no

Når `./bin/nc local-green --strict` er grøn, er normalflata lokalt kontrollert: release-preflight, aktiv flate, fase-0, L1-L6-sjølvstendighet og full testflate køyrer utan Python/C som aktiv arbeidsveg. Shell-wrapperar finst framleis for operativsystemgrensene og for fallback når native runtime manglar `exec_prosess`, men eigarlogikken ligg i `.no`-filer.

Den einaste aktive kjeldebrua utanfor Norscode er macOS AppKit/WebKit-hosten i `platform/macos/window-host/Main.swift`, med `Main.no` og Norscode-byggar ved sida av.

## Lisens

Apache-2.0. Sjå [LICENSE](LICENSE).

## Forfattar

Jan Steinar Sætre

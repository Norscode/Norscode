# Start Her

Dette er den korteste veien inn i Norscode for nye brukere.

## Først

Hvis du bare vil komme i gang, gjør dette:

```bash
./bin/nc --help
./bin/nc test
```

Hvis du ikke har bygget binary ennå, les først:

- [docs/INDEX.md](INDEX.md)
- [docs/WINDOWS.md](WINDOWS.md)
- [docs/CLI_CONTRACT.md](CLI_CONTRACT.md)

## Installer

På macOS og Linux bruker du vanligvis:

```bash
curl -fsSL https://raw.githubusercontent.com/Norscode/Norscode/main/tools/install.sh | sh
```

På Windows:

```powershell
irm https://raw.githubusercontent.com/Norscode/Norscode/main/tools/install.ps1 | iex
```

Hvis du vil bygge fra kildekode:

```bash
bash tools/build_norscode_native.sh
```

Dette brukar stage-0-seed og krev ikkje C-verktøykjede i normal bruk. Maintainer-regen er ein separat, eksplisitt vedlikehaldsløype.

## Ditt første program

```norscode
funksjon start() -> heltall {
    skriv("Hei, Norscode!")
    returner 0
}
```

Kjør det med:

```bash
./bin/nc run min.no
```

## De viktigste kommandoene

- `./bin/nc check fil.no` validerer kode uten å kjøre den
- `./bin/nc build fil.no ut.ncb.json` bygger NCB JSON
- `./bin/nc bygg --mål elf64 fil.no ut.elf` bygger native ELF
- `./bin/nc format fil.no` formaterer kode
- `./bin/nc lint fil.no` finner vanlige problemer
- `./bin/nc test` kjører testpakken
- `./bin/nc commands` viser den stabile CLI-kontrakten

## Lær videre

- [docs/COOKBOOK.md](COOKBOOK.md)
- [docs/EXAMPLES.md](EXAMPLES.md)
- [docs/PACKAGES.md](PACKAGES.md)
- [docs/FRONTEND_LEARNING_PATH.md](FRONTEND_LEARNING_PATH.md)
- [docs/SELFHOST_HANDLINGSPLAN.md](SELFHOST_HANDLINGSPLAN.md)
- [docs/LANE_MAP.md](LANE_MAP.md)
- [docs/ARCHIVE_INDEX.md](ARCHIVE_INDEX.md)

# Installere Norscode

Dette er kortversjonen for å installere Norscode lokalt utan Python- eller C-basert byggeløype.

## Rask installering

På macOS og Linux:

```bash
curl -fsSL https://raw.githubusercontent.com/Norscode/Norscode/main/tools/install.sh | sh
```

Legg deretter installasjonsmappa på `PATH` dersom ho ikkje alt ligg der:

```bash
export PATH="$HOME/.local/bin:$PATH"
```

Kontroller installasjonen:

```bash
nc --version
nc doctor
nc verify-seed
```

## Installere frå lokal kjeldekode

Frå repo-rota:

```bash
./bin/nc run tools/install.no
```

Dette brukar `dist/norscode_native` lokalt og installerer via Norscode-eigarlogikken.
POSIX-bootstrapen `sh tools/install.sh` finst framleis som førsteinstallasjonsveg
før Norscode-runtime er materialisert på maskina. Installert kommando blir:

```bash
~/.local/bin/nc
~/.local/bin/norscode
```

## Installere ferdig releasepakke

Norscode kan også distribuerast som éi enkel releasefil.

Språk-/repo-pakke:

```bash
./bin/nc package-release 0.1.0
./bin/nc install-release release-artifacts/norscode-language-0.1.0.tar.gz
```

Plattformpakkar blir lagt i `release-artifacts/` når dei blir bygde:

- `Norscode-macos-<versjon>.pkg`
- `Norscode-linux-<versjon>-AppDir.tar.gz`
- `Norscode-linux-<versjon>.AppImage`, dersom AppImage-verktøy er installert
- `norscode-windows-<versjon>.zip`, når Windows-artefakt finst

## Byggje lokale pakkar

macOS:

```bash
./bin/nc build-macos-app
./bin/nc package-macos-app --format all
```

Linux:

```bash
./bin/nc package-linux-app --format all
```

Windows-pakke rundt eksisterande `.exe`:

```bash
./bin/nc package-windows-app build/windows/norscode.exe
```

## Normal bruk etter installering

```bash
nc run app.no
nc check app.no
nc startproject mittprosjekt
nc test
nc feature-check app.no
```

Normal installasjon og normal bruk skal gå via `nc`, `norscode`, `dist/norscode_native` og selfhost-runtime. Historiske C- og Python-løyper er ikkje del av normal installasjon.

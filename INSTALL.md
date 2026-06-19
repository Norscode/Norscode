# Installere Norscode

Dette er kortversjonen for å installere Norscode lokalt utan Python- eller C-basert byggeløype.

## Rask Installering

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

## Installere Frå Lokal Kildekode

Frå repo-rota:

```bash
bash tools/install.sh
```

Dette brukar `dist/norscode_native` dersom han finst lokalt. Installert kommando blir:

```bash
~/.local/bin/nc
~/.local/bin/norscode
```

## Installere Ferdig Releasepakke

Norscode kan også distribuerast som éi enkel releasefil.

Språk-/repo-pakke:

```bash
bash package-release.sh 0.1.0
bash tools/install-release.sh release-artifacts/norscode-language-0.1.0.tar.gz
```

Plattformpakker blir lagt i `release-artifacts/` når dei blir bygde:

- `Norscode-macos-<versjon>.pkg`
- `Norscode-linux-<versjon>-AppDir.tar.gz`
- `Norscode-linux-<versjon>.AppImage`, dersom AppImage-verktøy er installert
- `norscode-windows-<versjon>.zip`, når Windows-artefakt finst

## Bygge Lokale Pakker

macOS:

```bash
bash tools/build-macos-app.sh
bash tools/package-macos-app.sh --format all
```

Linux:

```bash
bash tools/package-linux-app.sh --format all
```

Windows-pakke rundt eksisterande `.exe`:

```bash
bash tools/package-windows-app.sh build/windows/norscode.exe
```

## Normal Bruk Etter Installering

```bash
nc run app.no
nc check app.no
nc test
nc feature-check app.no
```

Normal installasjon og normal bruk skal gå via `nc`, `norscode`, `dist/norscode_native` og selfhost-runtime. Historiske C- og Python-løyper er ikkje del av normal installasjon.

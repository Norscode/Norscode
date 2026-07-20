# Norscode på Windows

## Rask installasjon

**Alternativ A – automatisk (PowerShell):**
```powershell
irm https://raw.githubusercontent.com/Norscode/Norscode/main/tools/install.ps1 | iex
```

**Alternativ B – fra kildekode:**
```powershell
git clone https://github.com/Norscode/Norscode
cd Norscode
./bin/nc run tools/build_norscode_native.no
```

## Kjøring

Etter installasjon:
```powershell
.\bin\nc.ps1 --help
.\bin\nc.ps1 run app.no
.\bin\nc.ps1 test
```

Direkte fra repoet:
```powershell
# Med bin/nc.ps1 (PowerShell, anbefalt):
.\bin\nc.ps1 run app.no

# Med bin/nc.cmd (CMD):
bin\nc.cmd run app.no
```

## Plattformstatus

| Funksjon                    | Status              |
|-----------------------------|---------------------|
| `bin\nc.ps1 run app.no`     | Ja                  |
| `bin\nc.ps1 test`           | Ja                  |
| `bin\nc.ps1 build`          | Ja                  |
| `bin/nc.cmd` og `bin/nc.ps1`| Ja                  |
| Native `.exe` (native-first)  | Delvis              |

## Krav

Windows-bruk går via `bin/nc.ps1` / `bin/nc.cmd` og stage-0/native-first-kjeda. Ingen normal Python-entrypoint er i bruk.

Den første Windows release-kontrakten for native `.exe`-sporet er no dokumentert i [docs/WINDOWS_APP_RELEASE.md](WINDOWS_APP_RELEASE.md), men sjølve `.exe`-produksjonen er framleis ikkje bevist ende til ende.

Den første repeterbare installasjonsløypa rundt ZIP-layouten er dokumentert i [docs/WINDOWS_APP_INSTALL.md](WINDOWS_APP_INSTALL.md).

Den første CI-linja rundt ZIP-layouten er dokumentert i [docs/WINDOWS_APP_CI.md](WINDOWS_APP_CI.md), men også ho står og ventar på repeterbar `norscode.exe`-produksjon.

Se README.md for full plattformoversikt.

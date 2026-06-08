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
bash tools/build_norscode_native.sh
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
| Native `.exe` (native-first)  | Planlagt            |

## Krav

Windows-bruk går via `bin/nc.ps1` / `bin/nc.cmd` og stage-0/native-first-kjeda. Ingen normal Python-entrypoint er i bruk.

Se README.md for full plattformoversikt.

# Norscode på Windows

## Rask installasjon

**Alternativ A – automatisk (PowerShell):**
```powershell
irm https://raw.githubusercontent.com/rfwwp8k542-maker/Norscode/main/tools/install.ps1 | iex
```

**Alternativ B – via pip:**
```powershell
py -m pip install norscode
```

**Alternativ C – fra kildekode:**
```powershell
git clone https://github.com/rfwwp8k542-maker/Norscode
cd Norscode
py -m pip install .
```

## Kjøring

Etter installasjon via pip:
```powershell
norcode --help
norcode run app.no
norcode test
```

Direkte fra kildekode (uten pip-installasjon):
```powershell
# Med bin/nc.ps1 (PowerShell, anbefalt):
.\bin\nc.ps1 run app.no

# Med bin/nc.cmd (CMD):
bin\nc.cmd run app.no

# Med historisk vei direkte:
py main.py run app.no
```

## Plattformstatus

| Funksjon                    | Status              |
|-----------------------------|---------------------|
| `norcode run app.no`        | Ja (via historisk vei/pip) |
| `norcode test`              | Ja (via historisk vei/pip) |
| `norcode build`             | Ja (via historisk vei/pip) |
| `bin/nc.cmd` og `bin/nc.ps1`| Ja                  |
| Native `.exe` (native-first)  | Planlagt            |

## Krav

Historisk bootstrap-lag 3.10 eller nyere. Installer via `install.ps1` eller den vanlige oppdateringsrutinen.

Se README.md for full plattformoversikt.

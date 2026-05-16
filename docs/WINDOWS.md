# Windows

Norscode kan installeres på Windows med vanlig `pip`-flyt.

## Anbefalt installasjon

For en publisert release:

```powershell
py -m pip install norcode
```

For en lokal utviklerkopi:

```powershell
py -m pip install .
```

## Kjøring

Etter installasjon kan du bruke:

```powershell
norcode --help
norcode check tests\test_web_dependency.no
```

Hvis du vil kjøre direkte fra kildekoden uten installasjon, bruk:

```powershell
py -m norcode --help
```

## Merknad

De Unix-spesifikke `bin/*`-skriptene brukes fortsatt på macOS og Linux. På Windows er `pip`-installerte console scripts den normale veien.

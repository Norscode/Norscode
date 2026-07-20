# Windows App Layout

Dette dokumentet skildrar den første Windows app-layouten for Norscode.

## Mål

Gi Windows-sporet ei meir stabil distribusjonsflate enn berre ein laus `norscode.exe`.

## Verktøy

- `tools/build-windows-app-layout.no`

Bruk:

```bash
NORSCODE_WINDOWS_EXE_PATH=path/to/norscode.exe ./bin/nc run tools/build-windows-app-layout.no
```

Output:

- `build/windows-app/Norscode/`

## Struktur

App-layouten inneheld:

- `bin/norscode.exe`
- `bin/nc.exe`
- `bin/nc.cmd`
- `bin/nc.ps1`
- `README.txt`
- `docs/LAYOUT.txt`

## Kvifor dette betyr noko

Dette er første steget frå:

- enkeltfil-binær

til:

- liten, ryddig Windows-appstruktur som seinare installasjon og oppgradering kan byggje vidare på

## Forhold til ZIP-release

[tools/package-windows-app.no](../tools/package-windows-app.no) pakkar no denne app-layouten inn i:

- `release-artifacts/norscode-windows-<versjon>.zip`

Så ZIP-en representerer no ikkje berre ein laus `.exe`, men ein liten appflate.
